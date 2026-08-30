/* --------------------------------------------------------------------  */
/*                          CALCULIX CGX                                 */
/*                   - MODERN 3D GRAPHICS ENGINE -                       */
/*                                                                       */
/*     cgx_vbo.c: Modern Vertex Buffer Object (VBO/VAO) Subsystem        */
/* --------------------------------------------------------------------  */

#include "cgx_vbo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

CgxMeshVBO* cgx_vbo_create(void)
{
    CgxMeshVBO *mesh = (CgxMeshVBO*)calloc(1, sizeof(CgxMeshVBO));
    if (!mesh) return NULL;

    glGenBuffers(1, &mesh->vbo_static);
    glGenBuffers(1, &mesh->vbo_dynamic);
    return mesh;
}

void cgx_vbo_destroy(CgxMeshVBO *mesh)
{
    if (!mesh) return;

    if (mesh->vbo_static) {
        glDeleteBuffers(1, &mesh->vbo_static);
        mesh->vbo_static = 0;
    }
    if (mesh->vbo_dynamic) {
        glDeleteBuffers(1, &mesh->vbo_dynamic);
        mesh->vbo_dynamic = 0;
    }
    if (mesh->node_indices) {
        free(mesh->node_indices);
        mesh->node_indices = NULL;
    }
    free(mesh);
}

/* Helper to add a triangle vertex to static buffer */
static inline void add_static_vertex(CgxStaticVertex *v_out, int *nodes_out, int *count,
                                     Nodes *node, int node_idx, double normal[3])
{
    int idx = *count;
    v_out[idx].x = (float)node[node_idx].nx;
    v_out[idx].y = (float)node[node_idx].ny;
    v_out[idx].z = (float)node[node_idx].nz;

    if (normal) {
        v_out[idx].nx = (float)normal[0];
        v_out[idx].ny = (float)normal[1];
        v_out[idx].nz = (float)normal[2];
    } else {
        v_out[idx].nx = 0.0f;
        v_out[idx].ny = 0.0f;
        v_out[idx].nz = 1.0f;
    }

    nodes_out[idx] = node_idx;
    (*count)++;
}

int cgx_vbo_build_static_faces(CgxMeshVBO *mesh, Faces *face, Nodes *node, int num_faces, int *face_indices)
{
    if (!mesh || !face || !node || num_faces <= 0) return 0;

    /* Estimate max triangles per face (up to 8 triangles for high-order faces) */
    int max_vertices = num_faces * 8 * 3;
    CgxStaticVertex *s_verts = (CgxStaticVertex*)malloc(max_vertices * sizeof(CgxStaticVertex));
    int *cached_nodes = (int*)malloc(max_vertices * sizeof(int));

    if (!s_verts || !cached_nodes) {
        if (s_verts) free(s_verts);
        if (cached_nodes) free(cached_nodes);
        return 0;
    }

    int v_count = 0;
    int i;

    for (i = 0; i < num_faces; i++) {
        int f_idx = face_indices ? face_indices[i] : i;
        Faces *f = &face[f_idx];
        double *n0 = f->side ? f->side[0] : NULL;

        switch (f->type) {
            case 7: /* 3-node Triangle */
                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[0], n0);
                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[1], n0);
                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[2], n0);
                break;

            case 8: /* 6-node Quadratic Triangle (4 sub-triangles) */
                /* Tri 1: (0, 3, 5) */
                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[0], n0);
                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[3], n0);
                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[5], n0);
                /* Tri 2: (3, 1, 4) */
                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[3], n0);
                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[1], n0);
                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[4], n0);
                /* Tri 3: (5, 4, 2) */
                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[5], n0);
                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[4], n0);
                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[2], n0);
                /* Tri 4: (3, 4, 5) */
                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[3], n0);
                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[4], n0);
                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[5], n0);
                break;

            case 9: /* 4-node Quad (2 sub-triangles) */
                /* Tri 1: (0, 1, 2) */
                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[0], n0);
                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[1], n0);
                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[2], n0);
                /* Tri 2: (2, 3, 0) */
                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[2], n0);
                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[3], n0);
                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[0], n0);
                break;

            case 10: /* 8-node Quadratic Quad (6 sub-triangles) */
                /* (0,4,7), (4,1,5), (7,6,3), (6,2,5), (4,5,6), (7,4,6) */
                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[0], n0);
                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[4], n0);
                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[7], n0);

                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[4], n0);
                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[1], n0);
                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[5], n0);

                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[7], n0);
                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[6], n0);
                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[3], n0);

                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[6], n0);
                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[2], n0);
                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[5], n0);

                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[4], n0);
                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[5], n0);
                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[6], n0);

                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[7], n0);
                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[4], n0);
                add_static_vertex(s_verts, cached_nodes, &v_count, node, f->nod[6], n0);
                break;

            default:
                break;
        }
    }

    mesh->vertex_count = v_count;

    /* Upload static buffer to GPU */
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo_static);
    glBufferData(GL_ARRAY_BUFFER, v_count * sizeof(CgxStaticVertex), s_verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (mesh->node_indices) free(mesh->node_indices);
    mesh->node_indices = cached_nodes;
    mesh->is_static_ready = 1;

    free(s_verts);
    return v_count;
}

int cgx_vbo_update_dynamic(CgxMeshVBO *mesh, double *colNr, Nodes *node, double *disp)
{
    if (!mesh || !mesh->is_static_ready || mesh->vertex_count <= 0 || !mesh->node_indices) {
        return 0;
    }

    CgxDynamicVertex *d_verts = (CgxDynamicVertex*)malloc(mesh->vertex_count * sizeof(CgxDynamicVertex));
    if (!d_verts) return 0;

    int i;
    for (i = 0; i < mesh->vertex_count; i++) {
        int n_idx = mesh->node_indices[i];
        
        /* Scalar Contour Value */
        if (colNr) {
            d_verts[i].scalar = (float)colNr[n_idx];
        } else {
            d_verts[i].scalar = 0.0f;
        }

        /* Displacement Vector */
        if (disp) {
            d_verts[i].dx = (float)disp[n_idx * 3 + 0];
            d_verts[i].dy = (float)disp[n_idx * 3 + 1];
            d_verts[i].dz = (float)disp[n_idx * 3 + 2];
        } else {
            d_verts[i].dx = 0.0f;
            d_verts[i].dy = 0.0f;
            d_verts[i].dz = 0.0f;
        }
    }

    /* Upload dynamic results buffer to GPU */
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo_dynamic);
    glBufferData(GL_ARRAY_BUFFER, mesh->vertex_count * sizeof(CgxDynamicVertex), d_verts, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    mesh->is_dynamic_ready = 1;
    free(d_verts);
    return 1;
}

void cgx_vbo_render_surface(CgxMeshVBO *mesh, mat4_t model, mat4_t view, mat4_t proj,
                            GLuint colormap_tex, float min_val, float max_val,
                            float disp_scale, int show_colormap)
{
    if (!mesh || !mesh->is_static_ready || mesh->vertex_count <= 0) return;

    CgxShaderProgram *prog = cgx_shaders_get_surface_program();
    if (!prog || !prog->program) return;

    glUseProgram(prog->program);

    /* Compute normal matrix (3x3 inverse transpose of model-view) */
    mat4_t model_view = mat4_multiply(view, model);
    float norm_mat[9];
    mat4_compute_normal_mat3(model_view, norm_mat);

    /* Set Uniforms */
    glUniformMatrix4fv(prog->loc_u_model, 1, GL_FALSE, model.m);
    glUniformMatrix4fv(prog->loc_u_view, 1, GL_FALSE, view.m);
    glUniformMatrix4fv(prog->loc_u_projection, 1, GL_FALSE, proj.m);
    glUniformMatrix3fv(prog->loc_u_normal_mat, 1, GL_FALSE, norm_mat);

    glUniform1f(prog->loc_u_displacement_scale, disp_scale);
    glUniform1i(prog->loc_u_use_displacement, (disp_scale != 0.0f) ? 1 : 0);

    glUniform1i(prog->loc_u_use_colormap, show_colormap ? 1 : 0);
    glUniform4f(prog->loc_u_base_color, 0.75f, 0.80f, 0.88f, 1.0f);
    glUniform3f(prog->loc_u_light_dir, 0.35f, 0.45f, 0.82f);
    glUniform3f(prog->loc_u_specular_color, 1.0f, 1.0f, 1.0f);
    glUniform1f(prog->loc_u_shininess, 96.0f);
    glUniform1f(prog->loc_u_specular_strength, 0.38f);
    glUniform1f(prog->loc_u_min_scalar, min_val);
    glUniform1f(prog->loc_u_max_scalar, max_val);

    /* Bind 1D Colormap Texture if active */
    if (show_colormap && colormap_tex) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colormap_tex);
        glUniform1i(prog->loc_u_colormap_tex, 0);
    }

    /* Bind Static Geometry VBO */
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo_static);
    if (prog->loc_in_position >= 0) {
        glEnableVertexAttribArray(prog->loc_in_position);
        glVertexAttribPointer(prog->loc_in_position, 3, GL_FLOAT, GL_FALSE, sizeof(CgxStaticVertex), (void*)0);
    }
    if (prog->loc_in_normal >= 0) {
        glEnableVertexAttribArray(prog->loc_in_normal);
        glVertexAttribPointer(prog->loc_in_normal, 3, GL_FLOAT, GL_FALSE, sizeof(CgxStaticVertex), (void*)(3 * sizeof(float)));
    }

    /* Bind Dynamic Results VBO (if available) */
    if (mesh->is_dynamic_ready && mesh->vbo_dynamic) {
        glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo_dynamic);
        if (prog->loc_in_scalar >= 0) {
            glEnableVertexAttribArray(prog->loc_in_scalar);
            glVertexAttribPointer(prog->loc_in_scalar, 1, GL_FLOAT, GL_FALSE, sizeof(CgxDynamicVertex), (void*)0);
        }
        if (prog->loc_in_displacement >= 0) {
            glEnableVertexAttribArray(prog->loc_in_displacement);
            glVertexAttribPointer(prog->loc_in_displacement, 3, GL_FLOAT, GL_FALSE, sizeof(CgxDynamicVertex), (void*)(sizeof(float)));
        }
    }

    /* Render Triangles */
    glDrawArrays(GL_TRIANGLES, 0, mesh->vertex_count);

    /* Cleanup attribute state */
    if (prog->loc_in_position >= 0) glDisableVertexAttribArray(prog->loc_in_position);
    if (prog->loc_in_normal >= 0) glDisableVertexAttribArray(prog->loc_in_normal);
    if (prog->loc_in_scalar >= 0) glDisableVertexAttribArray(prog->loc_in_scalar);
    if (prog->loc_in_displacement >= 0) glDisableVertexAttribArray(prog->loc_in_displacement);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
}
