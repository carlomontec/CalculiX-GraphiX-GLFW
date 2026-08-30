/* --------------------------------------------------------------------  */
/*                          CALCULIX CGX                                 */
/*                   - MODERN 3D GRAPHICS ENGINE -                       */
/*                                                                       */
/*     cgx_vbo.h: Modern Vertex Buffer Object (VBO/VAO) Subsystem        */
/* --------------------------------------------------------------------  */

#ifndef CGX_VBO_H
#define CGX_VBO_H

#include "cgx_glut_glfw.h"
#include "cgx_math.h"
#include "cgx_shaders.h"
#include "extUtil.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Static Geometry Vertex (Uploaded once per mesh geometry build) */
typedef struct {
    float x, y, z;       /* Position */
    float nx, ny, nz;    /* Normal vector */
} CgxStaticVertex;

/* Dynamic Results Vertex (Uploaded when switching datasets / loadcases) */
typedef struct {
    float scalar;        /* Contour scalar value */
    float dx, dy, dz;    /* Displacement vector */
} CgxDynamicVertex;

/* Modern Mesh VBO Container */
typedef struct {
    GLuint vao;
    GLuint vbo_static;
    GLuint vbo_dynamic;
    
    GLsizei vertex_count;
    int is_static_ready;
    int is_dynamic_ready;

    /* Cached node indices for fast dynamic dataset updates */
    int *node_indices;
} CgxMeshVBO;

/* Global flag for Modern GPU rendering pipeline (1 = ON, 0 = Legacy glCallList) */
extern int modernGpuFlag;

/* Global active mesh VBO instance */
extern CgxMeshVBO *g_active_mesh_vbo;

/* Create an empty mesh VBO container */
CgxMeshVBO* cgx_vbo_create(void);

/* Free mesh VBO container and GPU resources */
void cgx_vbo_destroy(CgxMeshVBO *mesh);

/* Build static geometry buffers from CGX Faces array */
int cgx_vbo_build_static_faces(CgxMeshVBO *mesh, Faces *face, Nodes *node, int num_faces, int *face_indices);

/* Update dynamic scalar & displacement dataset results */
int cgx_vbo_update_dynamic(CgxMeshVBO *mesh, double *colNr, Nodes *node, double *disp);

/* Synchronize active mesh VBO directly from CGX display psets */
int cgx_vbo_sync_from_psets(char key, Nodes *node, Faces *face, void *set_ptr, void *pset_ptr, int num_psets, double *colNr);

/* Update 1D/2D colormap GPU texture from float RGBA data */
void cgx_vbo_update_colormap_texture(float *rgba_pixels, int count);

/* Check if active mesh VBO is ready for rendering */
int cgx_vbo_is_ready(void);

/* Render the active mesh using modern shaders and current OpenGL view/projection matrices */
void cgx_vbo_render_active(int is_load_mode, float min_val, float max_val);

/* Render modern FEA mesh surface using shaders */
void cgx_vbo_render_surface(CgxMeshVBO *mesh, mat4_t model, mat4_t view, mat4_t proj,
                            GLuint colormap_tex, float min_val, float max_val,
                            float disp_scale, int show_colormap, int use_lighting);

#ifdef __cplusplus
}
#endif

#endif /* CGX_VBO_H */
