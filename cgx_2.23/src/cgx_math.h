/* --------------------------------------------------------------------  */
/*                          CALCULIX CGX                                 */
/*                   - MODERN 3D GRAPHICS ENGINE -                       */
/*                                                                       */
/*     cgx_math.h: Lightweight 4x4 Matrix & 3D Vector Math Library      */
/* --------------------------------------------------------------------  */

#ifndef CGX_MATH_H
#define CGX_MATH_H

#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define CGX_DEG2RAD(d) ((float)((d) * (M_PI / 180.0)))
#define CGX_RAD2DEG(r) ((float)((r) * (180.0 / M_PI)))

typedef struct {
    float m[16]; /* Column-major 4x4 matrix (OpenGL layout) */
} mat4_t;

typedef struct {
    float x, y, z;
} vec3_t;

/* Identity Matrix */
static inline mat4_t mat4_identity(void) {
    mat4_t r;
    memset(r.m, 0, sizeof(r.m));
    r.m[0] = 1.0f;
    r.m[5] = 1.0f;
    r.m[10] = 1.0f;
    r.m[15] = 1.0f;
    return r;
}

/* Matrix Multiplication: C = A * B */
static inline mat4_t mat4_multiply(mat4_t a, mat4_t b) {
    mat4_t r;
    int i, j, k;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            float sum = 0.0f;
            for (k = 0; k < 4; k++) {
                sum += a.m[k * 4 + j] * b.m[i * 4 + k];
            }
            r.m[i * 4 + j] = sum;
        }
    }
    return r;
}

/* Perspective Projection (Right-handed, matching gluPerspective) */
static inline mat4_t mat4_perspective(float fovy_deg, float aspect, float z_near, float z_far) {
    mat4_t r;
    float tan_half_fovy = tanf(CGX_DEG2RAD(fovy_deg) / 2.0f);
    memset(r.m, 0, sizeof(r.m));
    
    r.m[0] = 1.0f / (aspect * tan_half_fovy);
    r.m[5] = 1.0f / tan_half_fovy;
    r.m[10] = -(z_far + z_near) / (z_far - z_near);
    r.m[11] = -1.0f;
    r.m[14] = -(2.0f * z_far * z_near) / (z_far - z_near);
    r.m[15] = 0.0f;
    return r;
}

/* Orthographic Projection (matching glOrtho) */
static inline mat4_t mat4_ortho(float left, float right, float bottom, float top, float z_near, float z_far) {
    mat4_t r;
    memset(r.m, 0, sizeof(r.m));
    
    r.m[0] = 2.0f / (right - left);
    r.m[5] = 2.0f / (top - bottom);
    r.m[10] = -2.0f / (z_far - z_near);
    r.m[12] = -(right + left) / (right - left);
    r.m[13] = -(top + bottom) / (top - bottom);
    r.m[14] = -(z_far + z_near) / (z_far - z_near);
    r.m[15] = 1.0f;
    return r;
}

/* Translation Matrix */
static inline mat4_t mat4_translate(float tx, float ty, float tz) {
    mat4_t r = mat4_identity();
    r.m[12] = tx;
    r.m[13] = ty;
    r.m[14] = tz;
    return r;
}

/* Scale Matrix */
static inline mat4_t mat4_scale(float sx, float sy, float sz) {
    mat4_t r = mat4_identity();
    r.m[0] = sx;
    r.m[5] = sy;
    r.m[10] = sz;
    return r;
}

/* Vector Normalization */
static inline vec3_t vec3_normalize(vec3_t v) {
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len > 1e-8f) {
        float inv = 1.0f / len;
        v.x *= inv;
        v.y *= inv;
        v.z *= inv;
    }
    return v;
}

/* Vector Cross Product */
static inline vec3_t vec3_cross(vec3_t a, vec3_t b) {
    vec3_t r;
    r.x = a.y * b.z - a.z * b.y;
    r.y = a.z * b.x - a.x * b.z;
    r.z = a.x * b.y - a.y * b.x;
    return r;
}

/* Vector Dot Product */
static inline float vec3_dot(vec3_t a, vec3_t b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

/* LookAt Matrix (matching gluLookAt) */
static inline mat4_t mat4_lookat(vec3_t eye, vec3_t center, vec3_t up) {
    vec3_t f, s, u;
    mat4_t r = mat4_identity();
    
    f.x = center.x - eye.x;
    f.y = center.y - eye.y;
    f.z = center.z - eye.z;
    f = vec3_normalize(f);
    
    s = vec3_cross(f, vec3_normalize(up));
    s = vec3_normalize(s);
    
    u = vec3_cross(s, f);
    
    r.m[0] = s.x;
    r.m[4] = s.y;
    r.m[8] = s.z;
    
    r.m[1] = u.x;
    r.m[5] = u.y;
    r.m[9] = u.z;
    
    r.m[2] = -f.x;
    r.m[6] = -f.y;
    r.m[10] = -f.z;
    
    r.m[12] = -vec3_dot(s, eye);
    r.m[13] = -vec3_dot(u, eye);
    r.m[14] =  vec3_dot(f, eye);
    return r;
}

/* Convert legacy double[4][4] trackball rotation matrix to column-major float[16] */
static inline mat4_t mat4_from_double4x4(double m[4][4]) {
    mat4_t r;
    int i, j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            r.m[j * 4 + i] = (float)m[i][j];
        }
    }
    return r;
}

/* Compute 3x3 normal matrix (transpose of inverse of top-left 3x3 of mat4) */
static inline void mat4_compute_normal_mat3(mat4_t m, float norm_mat[9]) {
    /* Extract 3x3 submatrix */
    float a00 = m.m[0], a01 = m.m[1], a02 = m.m[2];
    float a10 = m.m[4], a11 = m.m[5], a12 = m.m[6];
    float a20 = m.m[8], a21 = m.m[9], a22 = m.m[10];

    float det = a00 * (a11 * a22 - a12 * a21) -
                a01 * (a10 * a22 - a12 * a20) +
                a02 * (a10 * a21 - a11 * a20);

    if (fabsf(det) < 1e-8f) {
        /* Fallback to identity 3x3 */
        memset(norm_mat, 0, 9 * sizeof(float));
        norm_mat[0] = 1.0f; norm_mat[4] = 1.0f; norm_mat[8] = 1.0f;
        return;
    }

    float inv_det = 1.0f / det;

    /* Inverse transpose (stored column-major) */
    norm_mat[0] =  (a11 * a22 - a12 * a21) * inv_det;
    norm_mat[1] = -(a10 * a22 - a12 * a20) * inv_det;
    norm_mat[2] =  (a10 * a21 - a11 * a20) * inv_det;

    norm_mat[3] = -(a01 * a22 - a02 * a21) * inv_det;
    norm_mat[4] =  (a00 * a22 - a02 * a20) * inv_det;
    norm_mat[5] = -(a00 * a21 - a01 * a20) * inv_det;

    norm_mat[6] =  (a01 * a12 - a02 * a11) * inv_det;
    norm_mat[7] = -(a00 * a12 - a02 * a10) * inv_det;
    norm_mat[8] =  (a00 * a11 - a01 * a10) * inv_det;
}

#endif /* CGX_MATH_H */
