#!/usr/bin/env python3
"""
CalculiX GraphiX (CGX) - Automated Test Suite Runner
=====================================================
Discovers and executes CGX test scripts headlessly (-bg mode) in isolated
temporary workspaces, verifying return codes, generated mesh topology,
boundary conditions, loads, sets, VTU outputs, and post-processing results.
"""

import os
import sys
import time
import shutil
import argparse
import subprocess
from pathlib import Path
from typing import List, Dict, Optional, Tuple

# ANSI Terminal Colors
class Color:
    RESET = "\033[0m"
    BOLD = "\033[1m"
    GREEN = "\033[32m"
    RED = "\033[31m"
    YELLOW = "\033[33m"
    CYAN = "\033[36m"
    GRAY = "\033[90m"

def colorize(text: str, color: str) -> str:
    if not sys.stdout.isatty():
        return text
    return f"{color}{text}{Color.RESET}"

# Path discovery
REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_TESTS_DIR = REPO_ROOT / "tests"
BUILD_TEST_RUN_DIR = REPO_ROOT / "build" / "test_run"

CANDIDATE_BINARIES = [
    REPO_ROOT / "bin" / "cgx_glfw",
    REPO_ROOT / "bin" / "cgx",
    REPO_ROOT / "cgx_glfw-macos-arm64",
    REPO_ROOT / "build" / "cgx_glfw",
]

# Custom test assertion registry
TEST_EXPECTATIONS: Dict[str, Dict] = {
    "test_hex_sweep_linear": {
        "files": ["all.msh"],
        "msh_contains": ["*ELEMENT, TYPE=C3D8"],
    },
    "test_hex_sweep_quadratic": {
        "files": ["all.msh"],
        "msh_contains": ["*ELEMENT, TYPE=C3D20"],
    },
    "test_hex_sweep_revolve": {
        "files": ["all.msh"],
        "msh_contains": ["*ELEMENT, TYPE=C3D20"],
    },
    "test_wedge_sweep_penta": {
        "files": ["all.msh"],
        "msh_contains": ["*ELEMENT, TYPE=C3D15"],
    },
    "test_tet4_linear": {
        "files": ["all.msh"],
        "msh_contains": ["*ELEMENT, TYPE=C3D4"],
    },
    "test_tet10_biased_beam": {
        "files": ["all.msh"],
        "msh_contains": ["*ELEMENT, TYPE=C3D10"],
    },
    "test_tet10_notch_bracket": {
        "files": ["all.msh"],
        "msh_contains": ["*ELEMENT, TYPE=C3D10"],
    },
    "test_tet_target_size": {
        "files": ["all.msh"],
        "msh_contains": ["*ELEMENT, TYPE=C3D10"],
    },
    "test_abaqus_export_all": {
        "files": ["all.msh", "fix_123.bou", "top.dlo", "tip.frc", "fix.nam", "tip.nam"],
        "file_contains": {
            "all.msh": ["*NODE", "*ELEMENT"],
            "fix_123.bou": ["BOUNDARY based on fix"],
            "top.dlo": ["Pressure based on top"],
            "tip.frc": ["Forces based on tip"],
            "fix.nam": ["*NSET,NSET=Nfix"],
            "tip.nam": ["*NSET,NSET=Ntip"],
        }
    },
    "test_contact_surfaces": {
        "files": ["all.msh", "master.sur", "slave.sur"],
        "file_contains": {
            "master.sur": ["*SURFACE, NAME=Smaster"],
            "slave.sur": ["*SURFACE, NAME=Sslave"],
        }
    },
    "test_vtu_export": {
        "files": ["all.vtu", "all.pvd", "all_step_001.vtu", "all_step_010.vtu"],
        "file_contains": {
            "all.vtu": ["<VTKFile", "<UnstructuredGrid>"],
            "all.pvd": ["<VTKFile type=\"Collection\"", "<Collection>"],
        }
    },
    "test_post_frd_subset_export": {
        "files": ["sub_nodes.frd"],
        "file_contains": {
            "sub_nodes.frd": ["1PSTEP", "100CL"],
        }
    },
    "test_post_raw_tabulation": {
        "files": ["tip_nodes.nam"],
        "file_contains": {
            "tip_nodes.nam": ["*NSET,NSET=Ntip_nodes"],
        }
    },
    "test_post_vtu_fields": {
        "files": ["all.vtu"],
        "file_contains": {
            "all.vtu": ["<VTKFile", "<UnstructuredGrid>", "<PointData"],
        }
    }
}

class TestResult:
    def __init__(self, name: str, category: str, script_path: Path):
        self.name = name
        self.category = category
        self.script_path = script_path
        self.passed = False
        self.skipped = False
        self.duration_ms = 0.0
        self.error_message: Optional[str] = None
        self.stdout = ""
        self.stderr = ""
        self.run_dir: Optional[Path] = None

def find_cgx_binary(explicit_path: Optional[str] = None) -> Optional[Path]:
    if explicit_path:
        p = Path(explicit_path).resolve()
        if p.exists() and os.access(p, os.X_OK):
            return p
        return None
    for cand in CANDIDATE_BINARIES:
        if cand.exists() and os.access(cand, os.X_OK):
            return cand
    which_path = shutil.which("cgx_glfw") or shutil.which("cgx")
    if which_path:
        return Path(which_path).resolve()
    return None

def discover_tests(tests_dir: Path, pattern: Optional[str] = None) -> List[Tuple[str, Path]]:
    tests = []
    for root, _, files in os.walk(tests_dir):
        rel_root = Path(root).relative_to(tests_dir)
        category = str(rel_root) if str(rel_root) != "." else "general"
        for f in sorted(files):
            if f.endswith(".fbl") or f.endswith(".fbd"):
                script_path = Path(root) / f
                test_name = script_path.stem
                full_test_id = f"{category}/{test_name}"
                if pattern is None or pattern.lower() in full_test_id.lower():
                    tests.append((category, script_path))
    return sorted(tests, key=lambda x: (x[0], x[1].name))

def validate_test_artifacts(test_name: str, run_dir: Path) -> Tuple[bool, Optional[str]]:
    expectations = TEST_EXPECTATIONS.get(test_name)
    if not expectations:
        # Generic check for dat export if test is test_post_dat_tabulation
        if test_name == "test_post_dat_tabulation":
            dat_files = list(run_dir.glob("*.dat"))
            if not dat_files or all(f.stat().st_size == 0 for f in dat_files):
                return False, "Expected non-empty *.dat tabulation file was not generated."
            return True, None
        return True, None

    # Check expected files
    for expected_file in expectations.get("files", []):
        fpath = run_dir / expected_file
        if not fpath.exists():
            return False, f"Expected artifact '{expected_file}' was not generated."
        if fpath.stat().st_size == 0:
            return False, f"Generated artifact '{expected_file}' is empty (0 bytes)."

    # Check msh contains
    if "msh_contains" in expectations:
        msh_path = run_dir / "all.msh"
        if msh_path.exists():
            content = msh_path.read_text(errors="replace")
            for needle in expectations["msh_contains"]:
                if needle not in content:
                    return False, f"Mesh file 'all.msh' missing expected pattern '{needle}'."

    # Check specific file contains
    if "file_contains" in expectations:
        for fname, patterns in expectations["file_contains"].items():
            fpath = run_dir / fname
            if fpath.exists():
                content = fpath.read_text(errors="replace")
                for needle in patterns:
                    if needle not in content:
                        return False, f"File '{fname}' missing expected content pattern '{needle}'."

    return True, None

def run_single_test(cgx_bin: Path, category: str, script_path: Path, verbose: bool, keep_artifacts: bool) -> TestResult:
    test_name = script_path.stem
    result = TestResult(test_name, category, script_path)
    
    # Isolated test run directory
    run_dir = BUILD_TEST_RUN_DIR / category / test_name
    if run_dir.exists():
        shutil.rmtree(run_dir)
    run_dir.mkdir(parents=True, exist_ok=True)
    result.run_dir = run_dir

    cmd = [str(cgx_bin), "-bg", str(script_path.resolve())]
    
    start_time = time.perf_counter()
    try:
        proc = subprocess.run(
            cmd,
            cwd=str(run_dir),
            capture_output=True,
            text=True,
            timeout=30.0
        )
        result.duration_ms = (time.perf_counter() - start_time) * 1000.0
        result.stdout = proc.stdout
        result.stderr = proc.stderr

        if proc.returncode != 0:
            result.passed = False
            result.error_message = f"Process exited with non-zero status code: {proc.returncode}"
            return result

        # Check for FATAL / SEGFAULT strings in output
        if "Segmentation fault" in proc.stderr or "core dumped" in proc.stderr:
            result.passed = False
            result.error_message = "Segmentation fault detected in execution output."
            return result

        # Validate file artifacts and contents
        valid, err = validate_test_artifacts(test_name, run_dir)
        if not valid:
            result.passed = False
            result.error_message = err
            return result

        result.passed = True

    except subprocess.TimeoutExpired:
        result.duration_ms = (time.perf_counter() - start_time) * 1000.0
        result.passed = False
        result.error_message = "Execution timed out (limit: 30s)."
    except Exception as e:
        result.duration_ms = (time.perf_counter() - start_time) * 1000.0
        result.passed = False
        result.error_message = f"Execution exception: {str(e)}"
    finally:
        if result.passed and not keep_artifacts and run_dir.exists():
            shutil.rmtree(run_dir, ignore_errors=True)

    return result

def main():
    parser = argparse.ArgumentParser(description="CalculiX GraphiX (CGX) Test Suite Runner")
    parser.add_argument("-k", "--filter", help="Run only tests matching pattern (e.g. 'tet', 'sweep', 'export', 'post')")
    parser.add_argument("-v", "--verbose", action="store_true", help="Print verbose output for each test")
    parser.add_argument("--bin", dest="cgx_bin", help="Explicit path to cgx_glfw binary")
    parser.add_argument("--keep-artifacts", action="store_true", help="Preserve temporary test artifacts in build/test_run/")
    parser.add_argument("--list", action="store_true", help="List all discovered tests and exit")
    args = parser.parse_args()

    print(colorize("\n" + "="*80, Color.BOLD))
    print(colorize("           CalculiX GraphiX (CGX) - Automated Test Suite Runner", Color.BOLD + Color.CYAN))
    print(colorize("="*80, Color.BOLD))

    cgx_bin = find_cgx_binary(args.cgx_bin)
    if not cgx_bin and not args.list:
        print(colorize("\n[ERROR] Could not find executable 'cgx_glfw' binary.", Color.RED + Color.BOLD))
        print("Please build the binary first:\n  make -C cgx_2.23/src -f Makefile.glfw -j4\n")
        sys.exit(1)

    if cgx_bin:
        print(f"CGX Binary   : {colorize(str(cgx_bin), Color.GREEN)}")
    print(f"Tests Root   : {DEFAULT_TESTS_DIR}")
    if args.filter:
        print(f"Filter       : {colorize(args.filter, Color.YELLOW)}")
    print("="*80 + "\n")

    test_list = discover_tests(DEFAULT_TESTS_DIR, args.filter)
    if not test_list:
        print(colorize("No tests found matching filter criteria.", Color.YELLOW))
        return

    if args.list:
        print(colorize(f"Discovered {len(test_list)} tests:", Color.BOLD))
        for cat, path in test_list:
            print(f"  - [{cat}] {path.stem} ({path.name})")
        return

    results: List[TestResult] = []
    total_start = time.perf_counter()

    current_cat = None
    for category, script_path in test_list:
        if category != current_cat:
            current_cat = category
            print(colorize(f"\n[Category: {current_cat}]", Color.BOLD + Color.CYAN))

        test_display = script_path.stem
        sys.stdout.write(f"  • {test_display:<45} ... ")
        sys.stdout.flush()

        res = run_single_test(cgx_bin, category, script_path, args.verbose, args.keep_artifacts)
        results.append(res)

        if res.passed:
            badge = colorize("[ PASS ]", Color.GREEN + Color.BOLD)
            duration_str = colorize(f"{res.duration_ms:6.1f} ms", Color.GRAY)
            print(f"{badge}  {duration_str}")
        else:
            badge = colorize("[ FAIL ]", Color.RED + Color.BOLD)
            duration_str = colorize(f"{res.duration_ms:6.1f} ms", Color.GRAY)
            print(f"{badge}  {duration_str}")
            if res.error_message:
                print(colorize(f"      Reason: {res.error_message}", Color.RED))
            if args.verbose or not res.passed:
                if res.stdout:
                    print(colorize("      --- stdout ---", Color.GRAY))
                    for line in res.stdout.strip().splitlines()[-10:]:
                        print(f"      {line}")
                if res.stderr:
                    print(colorize("      --- stderr ---", Color.GRAY))
                    for line in res.stderr.strip().splitlines()[-10:]:
                        print(f"      {line}")

    total_duration = time.perf_counter() - total_start
    passed_count = sum(1 for r in results if r.passed)
    failed_count = sum(1 for r in results if not r.passed and not r.skipped)
    skipped_count = sum(1 for r in results if r.skipped)

    print("\n" + "="*80)
    print(colorize("TEST EXECUTION SUMMARY", Color.BOLD))
    print("="*80)
    print(f"Total Tests : {len(results)}")
    print(f"Passed      : {colorize(str(passed_count), Color.GREEN if passed_count == len(results) else Color.BOLD)}")
    print(f"Failed      : {colorize(str(failed_count), Color.RED if failed_count > 0 else Color.GRAY)}")
    if skipped_count:
        print(f"Skipped     : {colorize(str(skipped_count), Color.YELLOW)}")
    print(f"Duration    : {total_duration:.2f} seconds")
    print("="*80)

    if failed_count > 0:
        print(colorize("\nFAILED TESTS:", Color.RED + Color.BOLD))
        for r in results:
            if not r.passed and not r.skipped:
                print(f"  - [{r.category}] {r.name}: {r.error_message}")
        print()
        sys.exit(1)
    else:
        print(colorize("\nAll tests passed successfully!\n", Color.GREEN + Color.BOLD))
        sys.exit(0)

if __name__ == "__main__":
    main()
