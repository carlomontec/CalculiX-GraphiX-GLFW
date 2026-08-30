/* --------------------------------------------------------------------  */
/*                          CALCULIX CGX                                 */
/*                   - MODERN 3D GRAPHICS ENGINE -                       */
/*                                                                       */
/*     cgx_shaders.c: Modern GLSL Shader Pipeline & Program Manager     */
/* --------------------------------------------------------------------  */

#define CGX_GL_LOADER_IMPLEMENTATION
#include "cgx_shaders.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static CgxShaderProgram g_surface_prog;
static int g_shaders_initialized = 0;

/* --------------------------------------------------------------------  */
/* GLSL Shaders: 3D FEA Surface with Metallic Shading & GPU Colormap     */
/* --------------------------------------------------------------------  */

static const char *g_vs_surface =
"#version 120\n"
"\n"
"attribute vec3 in_position;\n"
"attribute vec3 in_normal;\n"
"attribute float in_scalar;\n"
"attribute vec3 in_displacement;\n"
"\n"
"uniform mat4 u_model;\n"
"uniform mat4 u_view;\n"
"uniform mat4 u_projection;\n"
"uniform mat3 u_normal_mat;\n"
"\n"
"uniform float u_displacement_scale;\n"
"uniform int u_use_displacement;\n"
"\n"
"varying vec3 v_normal;\n"
"varying vec3 v_pos_view;\n"
"varying float v_scalar;\n"
"\n"
"void main()\n"
"{\n"
"    vec3 pos = in_position;\n"
"    if (u_use_displacement != 0) {\n"
"        pos += in_displacement * u_displacement_scale;\n"
"    }\n"
"\n"
"    vec4 pos_world = u_model * vec4(pos, 1.0);\n"
"    vec4 pos_view  = u_view * pos_world;\n"
"    gl_Position    = u_projection * pos_view;\n"
"\n"
"    v_normal   = normalize(u_normal_mat * in_normal);\n"
"    v_pos_view = pos_view.xyz;\n"
"    v_scalar   = in_scalar;\n"
"}\n";

static const char *g_fs_surface =
"#version 120\n"
"\n"
"varying vec3 v_normal;\n"
"varying vec3 v_pos_view;\n"
"varying float v_scalar;\n"
"\n"
"uniform sampler2D u_colormap_tex;\n"
"uniform int u_use_colormap;\n"
"uniform int u_use_lighting;\n"
"uniform vec4 u_base_color;\n"
"uniform vec3 u_light_dir;\n"
"uniform vec3 u_specular_color;\n"
"uniform float u_shininess;\n"
"uniform float u_specular_strength;\n"
"uniform float u_min_scalar;\n"
"uniform float u_max_scalar;\n"
"\n"
"void main()\n"
"{\n"
"    /* Base Albedo Color */\n"
"    vec4 albedo = u_base_color;\n"
"    if (u_use_colormap != 0) {\n"
"        float norm_s = clamp(v_scalar, 0.0, 1.0);\n"
"        albedo = texture2D(u_colormap_tex, vec2(norm_s, 0.5));\n"
"    }\n"
"\n"
"    if (u_use_lighting == 0) {\n"
"        gl_FragColor = albedo;\n"
"        return;\n"
"    }\n"
"\n"
"    vec3 N = normalize(v_normal);\n"
"    vec3 L = normalize(u_light_dir);\n"
"    vec3 V = normalize(-v_pos_view);\n"
"    vec3 H = normalize(L + V);\n"
"\n"
"    /* Two-sided normal for FEA surface rendering */\n"
"    if (!gl_FrontFacing) {\n"
"        N = -N;\n"
"    }\n"
"\n"
"    /* Ambient & Diffuse */\n"
"    float NdotL = max(dot(N, L), 0.0);\n"
"    vec3 ambient = 0.22 * albedo.rgb;\n"
"    vec3 diffuse = 0.78 * NdotL * albedo.rgb;\n"
"\n"
"    /* Polished Metallic Specular */\n"
"    float NdotH = max(dot(N, H), 0.0);\n"
"    float spec_factor = pow(NdotH, u_shininess);\n"
"    vec3 specular = u_specular_strength * spec_factor * u_specular_color;\n"
"\n"
"    gl_FragColor = vec4(ambient + diffuse + specular, albedo.a);\n"
"}\n";

/* --------------------------------------------------------------------  */
/* Shader Compilation Utilities                                          */
/* --------------------------------------------------------------------  */

static GLuint compile_shader(GLenum type, const char *src)
{
    GLuint shader = glCreateShader(type);
    GLint success;
    char log[1024];

    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        fprintf(stderr, "[CGX-GLSL] ERROR: Shader compilation failed (%s):\n%s\n",
                (type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT"), log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint cgx_create_shader_program(const char *vs_src, const char *fs_src)
{
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vs_src);
    if (!vs) return 0;

    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fs_src);
    if (!fs) {
        glDeleteShader(vs);
        return 0;
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint success;
    char log[1024];
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(prog, sizeof(log), NULL, log);
        fprintf(stderr, "[CGX-GLSL] ERROR: Program linking failed:\n%s\n", log);
        glDeleteProgram(prog);
        glDeleteShader(vs);
        glDeleteShader(fs);
        return 0;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

static void query_surface_locations(CgxShaderProgram *p)
{
    p->loc_u_model       = glGetUniformLocation(p->program, "u_model");
    p->loc_u_view        = glGetUniformLocation(p->program, "u_view");
    p->loc_u_projection  = glGetUniformLocation(p->program, "u_projection");
    p->loc_u_normal_mat  = glGetUniformLocation(p->program, "u_normal_mat");

    p->loc_u_displacement_scale = glGetUniformLocation(p->program, "u_displacement_scale");
    p->loc_u_use_displacement  = glGetUniformLocation(p->program, "u_use_displacement");

    p->loc_u_colormap_tex      = glGetUniformLocation(p->program, "u_colormap_tex");
    p->loc_u_use_colormap      = glGetUniformLocation(p->program, "u_use_colormap");
    p->loc_u_use_lighting      = glGetUniformLocation(p->program, "u_use_lighting");
    p->loc_u_base_color        = glGetUniformLocation(p->program, "u_base_color");
    p->loc_u_light_dir         = glGetUniformLocation(p->program, "u_light_dir");
    p->loc_u_specular_color    = glGetUniformLocation(p->program, "u_specular_color");
    p->loc_u_shininess         = glGetUniformLocation(p->program, "u_shininess");
    p->loc_u_specular_strength = glGetUniformLocation(p->program, "u_specular_strength");
    p->loc_u_min_scalar        = glGetUniformLocation(p->program, "u_min_scalar");
    p->loc_u_max_scalar        = glGetUniformLocation(p->program, "u_max_scalar");

    p->loc_in_position     = glGetAttribLocation(p->program, "in_position");
    p->loc_in_normal       = glGetAttribLocation(p->program, "in_normal");
    p->loc_in_scalar       = glGetAttribLocation(p->program, "in_scalar");
    p->loc_in_displacement = glGetAttribLocation(p->program, "in_displacement");
}

int cgx_shaders_init(void)
{
    if (g_shaders_initialized) return 1;

    cgx_gl_load_extensions();

    memset(&g_surface_prog, 0, sizeof(g_surface_prog));
    g_surface_prog.program = cgx_create_shader_program(g_vs_surface, g_fs_surface);
    if (!g_surface_prog.program) {
        fprintf(stderr, "[CGX-GLSL] Warning: Failed to initialize Modern GLSL surface program.\n");
        return 0;
    }

    query_surface_locations(&g_surface_prog);
    g_shaders_initialized = 1;
    printf("[CGX-GLSL] Initialized Modern GLSL 3D Surface Shader Program (ID: %u)\n", g_surface_prog.program);
    return 1;
}

void cgx_shaders_cleanup(void)
{
    if (g_surface_prog.program) {
        glDeleteProgram(g_surface_prog.program);
        g_surface_prog.program = 0;
    }
    g_shaders_initialized = 0;
}

CgxShaderProgram* cgx_shaders_get_surface_program(void)
{
    if (!g_shaders_initialized) {
        cgx_shaders_init();
    }
    return &g_surface_prog;
}
