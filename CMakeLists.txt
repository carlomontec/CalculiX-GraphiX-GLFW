cmake_minimum_required(VERSION 3.20)

project(CalculiX-GraphiX-GLFW
    VERSION 2.23.0
    DESCRIPTION "CalculiX GraphiX (CGX) - Modern GLFW Edition"
    LANGUAGES C CXX
)

# Standard C/C++ language levels
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output binaries directly into top-level bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

# Options
if(APPLE)
    option(STATIC_GLFW "Link GLFW statically for standalone binary deployment" ON)
else()
    option(STATIC_GLFW "Link GLFW statically for standalone binary deployment" OFF)
endif()

# -----------------------------------------------------------------------------
# Source Files Configuration
# -----------------------------------------------------------------------------
set(CGX_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cgx_2.23/src")
set(LIBSNL_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libSNL/src")

set(CGX_C_SOURCES
    ${CGX_SRC_DIR}/AsplitA.c
    ${CGX_SRC_DIR}/AsplitL.c
    ${CGX_SRC_DIR}/adjustMidsideNode.c
    ${CGX_SRC_DIR}/compareStrings.c
    ${CGX_SRC_DIR}/XFunktions.c
    ${CGX_SRC_DIR}/badelems.c
    ${CGX_SRC_DIR}/bodyMesh2.c
    ${CGX_SRC_DIR}/bsort.c
    ${CGX_SRC_DIR}/bsortf.c
    ${CGX_SRC_DIR}/bsorti.c
    ${CGX_SRC_DIR}/calcNormalen.c
    ${CGX_SRC_DIR}/calcPrinc.c
    ${CGX_SRC_DIR}/calcPvector.c
    ${CGX_SRC_DIR}/calcWeight.c
    ${CGX_SRC_DIR}/checkIfNumber.c
    ${CGX_SRC_DIR}/cgx.c
    ${CGX_SRC_DIR}/compare.c
    ${CGX_SRC_DIR}/copyEntity.c
    ${CGX_SRC_DIR}/corrad.c
    ${CGX_SRC_DIR}/dataGeo.c
    ${CGX_SRC_DIR}/dataMesh.c
    ${CGX_SRC_DIR}/defineEntity.c
    ${CGX_SRC_DIR}/dispLists.c
    ${CGX_SRC_DIR}/elemChecker.c
    ${CGX_SRC_DIR}/extFunktions.c
    ${CGX_SRC_DIR}/extGL.c
    ${CGX_SRC_DIR}/fillBody2.c
    ${CGX_SRC_DIR}/frecord.c
    ${CGX_SRC_DIR}/foamFaces.c
    ${CGX_SRC_DIR}/gl3grades.c
    ${CGX_SRC_DIR}/getGeoDataTria.c
    ${CGX_SRC_DIR}/graph.c
    ${CGX_SRC_DIR}/ifind.c
    ${CGX_SRC_DIR}/iinsert.c
    ${CGX_SRC_DIR}/improveBadTr3.c
    ${CGX_SRC_DIR}/improveMesh.c
    ${CGX_SRC_DIR}/iniMeshData.c
    ${CGX_SRC_DIR}/intpol.c
    ${CGX_SRC_DIR}/intpol2.c
    ${CGX_SRC_DIR}/intpol3.c
    ${CGX_SRC_DIR}/iremove.c
    ${CGX_SRC_DIR}/linelength.c
    ${CGX_SRC_DIR}/m_copy.c
    ${CGX_SRC_DIR}/m_sub.c
    ${CGX_SRC_DIR}/mergEntity.c
    ${CGX_SRC_DIR}/mesh2d.c
    ${CGX_SRC_DIR}/meshSet.c
    ${CGX_SRC_DIR}/meshSurf.c
    ${CGX_SRC_DIR}/messages.c
    ${CGX_SRC_DIR}/near3d.c
    ${CGX_SRC_DIR}/normdist.c
    ${CGX_SRC_DIR}/nurbl2seq.c
    ${CGX_SRC_DIR}/orient.c
    ${CGX_SRC_DIR}/p_angle.c
    ${CGX_SRC_DIR}/parser.c
    ${CGX_SRC_DIR}/pickFunktions.c
    ${CGX_SRC_DIR}/plotFunktions.c
    ${CGX_SRC_DIR}/readAnsysList.c
    ${CGX_SRC_DIR}/readDynaList.c
    ${CGX_SRC_DIR}/readEdges.c
    ${CGX_SRC_DIR}/readfrd.c
    ${CGX_SRC_DIR}/readFoam.c
    ${CGX_SRC_DIR}/readStdCmap.c
    ${CGX_SRC_DIR}/readStl.c
    ${CGX_SRC_DIR}/readVtk.c
    ${CGX_SRC_DIR}/readccx.c
    ${CGX_SRC_DIR}/readDuns.c
    ${CGX_SRC_DIR}/readNastran.c
    ${CGX_SRC_DIR}/readNG.c
    ${CGX_SRC_DIR}/readTG.c
    ${CGX_SRC_DIR}/readIsaac.c
    ${CGX_SRC_DIR}/readstep.c
    ${CGX_SRC_DIR}/readWf.c
    ${CGX_SRC_DIR}/rectcyl.c
    ${CGX_SRC_DIR}/renumberfrd.c
    ${CGX_SRC_DIR}/selectDisplayFaces.c
    ${CGX_SRC_DIR}/sendMpc.c
    ${CGX_SRC_DIR}/sendSet.c
    ${CGX_SRC_DIR}/setFunktions.c
    ${CGX_SRC_DIR}/shapeFunctions.c
    ${CGX_SRC_DIR}/spline.c
    ${CGX_SRC_DIR}/splitElementsToTets.c
    ${CGX_SRC_DIR}/stof.c
    ${CGX_SRC_DIR}/stoi.c
    ${CGX_SRC_DIR}/stopClock.c
    ${CGX_SRC_DIR}/stos.c
    ${CGX_SRC_DIR}/strfind.c
    ${CGX_SRC_DIR}/strsplt.c
    ${CGX_SRC_DIR}/sins.c
    ${CGX_SRC_DIR}/surfMesh2.c
    ${CGX_SRC_DIR}/sword.c
    ${CGX_SRC_DIR}/trackball.c
    ${CGX_SRC_DIR}/v_distA.c
    ${CGX_SRC_DIR}/v_rec2cyl.c
    ${CGX_SRC_DIR}/v_add.c
    ${CGX_SRC_DIR}/v_angle.c
    ${CGX_SRC_DIR}/v_angle_ref.c
    ${CGX_SRC_DIR}/v_betrag.c
    ${CGX_SRC_DIR}/v_matmult.c
    ${CGX_SRC_DIR}/v_norm.c
    ${CGX_SRC_DIR}/v_prod.c
    ${CGX_SRC_DIR}/v_result.c
    ${CGX_SRC_DIR}/v_scal.c
    ${CGX_SRC_DIR}/v_rot.c
    ${CGX_SRC_DIR}/v_sprod.c
    ${CGX_SRC_DIR}/m_prod.c
    ${CGX_SRC_DIR}/m_prodtr.c
    ${CGX_SRC_DIR}/write2aba.c
    ${CGX_SRC_DIR}/write2ansys.c
    ${CGX_SRC_DIR}/write2aster.c
    ${CGX_SRC_DIR}/write2darwin.c
    ${CGX_SRC_DIR}/write2dolfyn.c
    ${CGX_SRC_DIR}/write2dyna.c
    ${CGX_SRC_DIR}/write2isaac.c
    ${CGX_SRC_DIR}/write2duns.c
    ${CGX_SRC_DIR}/write2frd.c
    ${CGX_SRC_DIR}/write2foam.c
    ${CGX_SRC_DIR}/write2vtu.c
    ${CGX_SRC_DIR}/write2nas.c
    ${CGX_SRC_DIR}/write2samcef.c
    ${CGX_SRC_DIR}/write2tochnog.c
    ${CGX_SRC_DIR}/writefbd.c
    ${CGX_SRC_DIR}/writebp.c
    ${CGX_SRC_DIR}/contact.c
    ${CGX_SRC_DIR}/makeTriFromElems.c
    ${CGX_SRC_DIR}/userFunction.c
    ${CGX_SRC_DIR}/cgx_capture.c
    ${CGX_SRC_DIR}/cgx_glut_glfw.c
    ${CGX_SRC_DIR}/cgx_iosevka_font.c
)

set(CGX_CXX_SOURCES
    ${CGX_SRC_DIR}/uselibSNL.cpp
    ${CGX_SRC_DIR}/generateTet.cpp
    ${CGX_SRC_DIR}/tetgen/tetgen.cxx
    ${CGX_SRC_DIR}/tetgen/predicates.cxx
)

file(GLOB LIBSNL_SOURCES "${LIBSNL_SRC_DIR}/*.cpp")

# -----------------------------------------------------------------------------
# Target Executable
# -----------------------------------------------------------------------------
add_executable(cgx_glfw
    ${CGX_C_SOURCES}
    ${CGX_CXX_SOURCES}
    ${LIBSNL_SOURCES}
)

target_include_directories(cgx_glfw PRIVATE
    ${CGX_SRC_DIR}
    ${CGX_SRC_DIR}/tetgen
    ${LIBSNL_SRC_DIR}
)

target_compile_definitions(cgx_glfw PRIVATE
    USE_GLFW
    TETLIBRARY
)

# Compiler Warnings and Flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(cgx_glfw PRIVATE
        -Wall
        -Wno-narrowing
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-register>
    )
endif()

# -----------------------------------------------------------------------------
# Dependency Resolution & Linking
# -----------------------------------------------------------------------------
find_package(OpenGL REQUIRED)

# Math & Threads
find_package(Threads REQUIRED)
target_link_libraries(cgx_glfw PRIVATE Threads::Threads)
if(UNIX AND NOT APPLE)
    target_link_libraries(cgx_glfw PRIVATE m dl)
elseif(APPLE)
    target_link_libraries(cgx_glfw PRIVATE m)
endif()

# OpenGL & GLU
if(TARGET OpenGL::GL)
    target_link_libraries(cgx_glfw PRIVATE OpenGL::GL)
else()
    target_link_libraries(cgx_glfw PRIVATE ${OPENGL_LIBRARIES})
endif()

if(TARGET OpenGL::GLU)
    target_link_libraries(cgx_glfw PRIVATE OpenGL::GLU)
endif()

# GLFW3
if(STATIC_GLFW AND APPLE)
    # Statically link GLFW on macOS for zero Homebrew runtime dependency
    execute_process(
        COMMAND brew --prefix glfw
        OUTPUT_VARIABLE BREW_GLFW_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    find_library(GLFW_STATIC_LIB NAMES libglfw3.a glfw3 PATHS "${BREW_GLFW_PREFIX}/lib" /opt/homebrew/lib /usr/local/lib NO_DEFAULT_PATH)
    if(NOT GLFW_STATIC_LIB)
        find_library(GLFW_STATIC_LIB NAMES libglfw3.a glfw3)
    endif()
    
    if(GLFW_STATIC_LIB)
        message(STATUS "Using static GLFW library: ${GLFW_STATIC_LIB}")
        target_link_libraries(cgx_glfw PRIVATE "${GLFW_STATIC_LIB}")
    else()
        message(FATAL_ERROR "STATIC_GLFW requested, but libglfw3.a was not found.")
    endif()

    find_path(GLFW_INCLUDE_DIR GLFW/glfw3.h PATHS "${BREW_GLFW_PREFIX}/include" /opt/homebrew/include /usr/local/include)
    if(GLFW_INCLUDE_DIR)
        target_include_directories(cgx_glfw PRIVATE "${GLFW_INCLUDE_DIR}")
    endif()
else()
    # Dynamic or standard discovery
    find_package(glfw3 3.3 CONFIG QUIET)
    if(glfw3_FOUND)
        target_link_libraries(cgx_glfw PRIVATE glfw)
    else()
        find_package(PkgConfig QUIET)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(GLFW REQUIRED glfw3)
            target_include_directories(cgx_glfw PRIVATE ${GLFW_INCLUDE_DIRS})
            target_link_libraries(cgx_glfw PRIVATE ${GLFW_LIBRARIES})
        else()
            find_path(GLFW_INCLUDE_DIR GLFW/glfw3.h)
            find_library(GLFW_LIBRARY NAMES glfw glfw3)
            target_include_directories(cgx_glfw PRIVATE ${GLFW_INCLUDE_DIR})
            target_link_libraries(cgx_glfw PRIVATE ${GLFW_LIBRARY})
        endif()
    endif()
endif()

# Platform-specific frameworks & flags
if(APPLE)
    find_library(COCOA_FRAMEWORK Cocoa REQUIRED)
    find_library(IOKIT_FRAMEWORK IOKit REQUIRED)
    find_library(COREVIDEO_FRAMEWORK CoreVideo REQUIRED)
    find_library(QUARTZCORE_FRAMEWORK QuartzCore REQUIRED)
    target_link_libraries(cgx_glfw PRIVATE
        ${COCOA_FRAMEWORK}
        ${IOKIT_FRAMEWORK}
        ${COREVIDEO_FRAMEWORK}
        ${QUARTZCORE_FRAMEWORK}
    )
elseif(WIN32)
    target_compile_definitions(cgx_glfw PRIVATE WIN32)
    target_link_libraries(cgx_glfw PRIVATE opengl32 glu32 gdi32 user32 kernel32)
    if(MINGW)
        target_link_options(cgx_glfw PRIVATE -static-libgcc -static-libstdc++ -static)
    endif()
endif()

# -----------------------------------------------------------------------------
# Installation Rules
# -----------------------------------------------------------------------------
install(TARGETS cgx_glfw RUNTIME DESTINATION bin)

# -----------------------------------------------------------------------------
# Testing Integration (CTest)
# -----------------------------------------------------------------------------
enable_testing()
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_FOUND)
    add_test(
        NAME cgx_automated_suite
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_tests.py --bin $<TARGET_FILE:cgx_glfw>
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()
