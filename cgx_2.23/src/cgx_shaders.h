/* --------------------------------------------------------------------  */
/*                          CALCULIX CGX                                 */
/*                   - MODERN 3D GRAPHICS ENGINE -                       */
/*                                                                       */
/*     cgx_shaders.h: Modern GLSL Shader Pipeline & Program Manager     */
/* --------------------------------------------------------------------  */

#ifndef CGX_SHADERS_H
#define CGX_SHADERS_H

#include "cgx_glut_glfw.h"
#include "cgx_math.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    GLuint program;
    GLuint vs;
    GLuint fs;

    /* Common Uniform Locations */
    GLint loc_u_model;
    GLint loc_u_view;
    GLint loc_u_projection;
    GLint loc_u_normal_mat;
    
    GLint loc_u_displacement_scale;
    GLint loc_u_use_displacement;
    GLint loc_u_use_lighting;

    GLint loc_u_colormap_tex;
    GLint loc_u_use_colormap;
    GLint loc_u_base_color;
    GLint loc_u_light_dir;
    GLint loc_u_specular_color;
    GLint loc_u_shininess;
    GLint loc_u_specular_strength;
    GLint loc_u_min_scalar;
    GLint loc_u_max_scalar;

    /* Attribute Locations */
    GLint loc_in_position;
    GLint loc_in_normal;
    GLint loc_in_scalar;
    GLint loc_in_displacement;
} CgxShaderProgram;

/* Initialize and compile standard CGX shaders */
int cgx_shaders_init(void);

/* Free all shader programs */
void cgx_shaders_cleanup(void);

/* Get the primary 3D FEA surface shader program */
CgxShaderProgram* cgx_shaders_get_surface_program(void);

/* Helper to compile a shader pair */
GLuint cgx_create_shader_program(const char *vs_src, const char *fs_src);

#ifdef __cplusplus
}
#endif

#endif /* CGX_SHADERS_H */
