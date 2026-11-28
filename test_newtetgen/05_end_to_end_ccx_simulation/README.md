# End-to-end example: TetGen C3D10 mesh → CalculiX static solve

A 100 × 10 × 10 mm steel cantilever, meshed with biased C3D10 tetrahedra by the embedded
TetGen engine. One end is fixed and a load of −15 N in y is applied at the tip.

The mesh and solver output are not stored in the repository (about 160 MB).
To regenerate them:

```bash
cd test_newtetgen/05_end_to_end_ccx_simulation

# 1. Mesh: writes all.msh (≈175k nodes, ≈122k C3D10 elements), fix_123.bou and tip.nam
../../bin/cgx_glfw -bg generate_biased_beam.fbl

# 2. Solve with CalculiX CrunchiX: writes beam_tet.frd, beam_tet.dat, beam_tet.sta, ...
ccx beam_tet

# 3. View the results
../../bin/cgx_glfw beam_tet.frd
```

| File | Content |
|------|---------|
| `generate_biased_beam.fbl` | Geometry, biased divisions (`bia 8.0` along the length), TetGen meshing, export |
| `generate_mesh.fbl` | Same beam with a coarser, uniform division |
| `beam_tet.inp` | Solver deck: material, boundary conditions, tip load, output requests |
| `fix_123.bou`, `tip.nam` | Fixed-end boundary and tip node set. They are written by step 1 and kept in the repository so the deck is complete. |
| `tip.dlo` | Placeholder pressure load file (not used by `beam_tet.inp`) |

On macOS (Apple Silicon), step 1 takes a few seconds and step 2 about 50 s with 8 threads.
The `beam_tet.json` result file is written only by CCX builds with JSON export.
Standard CCX writes `.frd` and `.dat`.
