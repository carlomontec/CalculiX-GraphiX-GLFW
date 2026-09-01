/* --------------------------------------------------------------------  */
/*                          CALCULIX CGX                                 */
/*                   - MODERN 3D GRAPHICS ENGINE -                       */
/*                                                                       */
/*     cgx_gl_loader.h: Minimal Cross-Platform OpenGL Extension Loader   */
/* --------------------------------------------------------------------  */

#ifndef CGX_GL_LOADER_H
#define CGX_GL_LOADER_H

#include "cgx_glut_glfw.h"

#if defined(_WIN32) || defined(WIN32)
#include <windows.h>
#include <stddef.h>

#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88E4
#endif
#ifndef GL_DYNAMIC_DRAW
#define GL_DYNAMIC_DRAW 0x88E8
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif

typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;
typedef char GLchar;

/* Function Pointer Types */
typedef GLuint (APIENTRY *PFNGLCREATESHADERPROC)(GLenum type);
typedef void   (APIENTRY *PFNGLSHADERSOURCEPROC)(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length);
typedef void   (APIENTRY *PFNGLCOMPILESHADERPROC)(GLuint shader);
typedef void   (APIENTRY *PFNGLGETSHADERIVPROC)(GLuint shader, GLenum pname, GLint* params);
typedef void   (APIENTRY *PFNGLGETSHADERINFOLOGPROC)(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
typedef void   (APIENTRY *PFNGLDELETESHADERPROC)(GLuint shader);
typedef GLuint (APIENTRY *PFNGLCREATEPROGRAMPROC)(void);
typedef void   (APIENTRY *PFNGLATTACHSHADERPROC)(GLuint program, GLuint shader);
typedef void   (APIENTRY *PFNGLLINKPROGRAMPROC)(GLuint program);
typedef void   (APIENTRY *PFNGLGETPROGRAMIVPROC)(GLuint program, GLenum pname, GLint* params);
typedef void   (APIENTRY *PFNGLGETPROGRAMINFOLOGPROC)(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
typedef void   (APIENTRY *PFNGLUSEPROGRAMPROC)(GLuint program);
typedef void   (APIENTRY *PFNGLDELETEPROGRAMPROC)(GLuint program);
typedef GLint  (APIENTRY *PFNGLGETUNIFORMLOCATIONPROC)(GLuint program, const GLchar* name);
typedef GLint  (APIENTRY *PFNGLGETATTRIBLOCATIONPROC)(GLuint program, const GLchar* name);
typedef void   (APIENTRY *PFNGLUNIFORMMATRIX4FVPROC)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
typedef void   (APIENTRY *PFNGLUNIFORMMATRIX3FVPROC)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
typedef void   (APIENTRY *PFNGLUNIFORM1FPROC)(GLint location, GLfloat v0);
typedef void   (APIENTRY *PFNGLUNIFORM1IPROC)(GLint location, GLint v0);
typedef void   (APIENTRY *PFNGLUNIFORM3FPROC)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void   (APIENTRY *PFNGLUNIFORM4FPROC)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void   (APIENTRY *PFNGLACTIVETEXTUREPROC)(GLenum texture);

typedef void   (APIENTRY *PFNGLGENBUFFERSPROC)(GLsizei n, GLuint* buffers);
typedef void   (APIENTRY *PFNGLBINDBUFFERPROC)(GLenum target, GLuint buffer);
typedef void   (APIENTRY *PFNGLBUFFERDATAPROC)(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
typedef void   (APIENTRY *PFNGLDELETEBUFFERSPROC)(GLsizei n, const GLuint* buffers);
typedef void   (APIENTRY *PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint index);
typedef void   (APIENTRY *PFNGLVERTEXATTRIBPOINTERPROC)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);
typedef void   (APIENTRY *PFNGLDISABLEVERTEXATTRIBARRAYPROC)(GLuint index);

#ifdef CGX_GL_LOADER_IMPLEMENTATION
#define CGX_GL_EXT(type, name) type name = NULL
#else
#define CGX_GL_EXT(type, name) extern type name
#endif

CGX_GL_EXT(PFNGLCREATESHADERPROC, glCreateShader);
CGX_GL_EXT(PFNGLSHADERSOURCEPROC, glShaderSource);
CGX_GL_EXT(PFNGLCOMPILESHADERPROC, glCompileShader);
CGX_GL_EXT(PFNGLGETSHADERIVPROC, glGetShaderiv);
CGX_GL_EXT(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog);
CGX_GL_EXT(PFNGLDELETESHADERPROC, glDeleteShader);
CGX_GL_EXT(PFNGLCREATEPROGRAMPROC, glCreateProgram);
CGX_GL_EXT(PFNGLATTACHSHADERPROC, glAttachShader);
CGX_GL_EXT(PFNGLLINKPROGRAMPROC, glLinkProgram);
CGX_GL_EXT(PFNGLGETPROGRAMIVPROC, glGetProgramiv);
CGX_GL_EXT(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog);
CGX_GL_EXT(PFNGLUSEPROGRAMPROC, glUseProgram);
CGX_GL_EXT(PFNGLDELETEPROGRAMPROC, glDeleteProgram);
CGX_GL_EXT(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation);
CGX_GL_EXT(PFNGLGETATTRIBLOCATIONPROC, glGetAttribLocation);
CGX_GL_EXT(PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv);
CGX_GL_EXT(PFNGLUNIFORMMATRIX3FVPROC, glUniformMatrix3fv);
CGX_GL_EXT(PFNGLUNIFORM1FPROC, glUniform1f);
CGX_GL_EXT(PFNGLUNIFORM1IPROC, glUniform1i);
CGX_GL_EXT(PFNGLUNIFORM3FPROC, glUniform3f);
CGX_GL_EXT(PFNGLUNIFORM4FPROC, glUniform4f);
CGX_GL_EXT(PFNGLACTIVETEXTUREPROC, glActiveTexture);

CGX_GL_EXT(PFNGLGENBUFFERSPROC, glGenBuffers);
CGX_GL_EXT(PFNGLBINDBUFFERPROC, glBindBuffer);
CGX_GL_EXT(PFNGLBUFFERDATAPROC, glBufferData);
CGX_GL_EXT(PFNGLDELETEBUFFERSPROC, glDeleteBuffers);
CGX_GL_EXT(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray);
CGX_GL_EXT(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer);
CGX_GL_EXT(PFNGLDISABLEVERTEXATTRIBARRAYPROC, glDisableVertexAttribArray);

static inline int cgx_gl_load_extensions(void) {
    static int loaded = 0;
    if (loaded) return 1;
    glCreateShader = (PFNGLCREATESHADERPROC)glfwGetProcAddress("glCreateShader");
    glShaderSource = (PFNGLSHADERSOURCEPROC)glfwGetProcAddress("glShaderSource");
    glCompileShader = (PFNGLCOMPILESHADERPROC)glfwGetProcAddress("glCompileShader");
    glGetShaderiv = (PFNGLGETSHADERIVPROC)glfwGetProcAddress("glGetShaderiv");
    glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)glfwGetProcAddress("glGetShaderInfoLog");
    glDeleteShader = (PFNGLDELETESHADERPROC)glfwGetProcAddress("glDeleteShader");
    glCreateProgram = (PFNGLCREATEPROGRAMPROC)glfwGetProcAddress("glCreateProgram");
    glAttachShader = (PFNGLATTACHSHADERPROC)glfwGetProcAddress("glAttachShader");
    glLinkProgram = (PFNGLLINKPROGRAMPROC)glfwGetProcAddress("glLinkProgram");
    glGetProgramiv = (PFNGLGETPROGRAMIVPROC)glfwGetProcAddress("glGetProgramiv");
    glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)glfwGetProcAddress("glGetProgramInfoLog");
    glUseProgram = (PFNGLUSEPROGRAMPROC)glfwGetProcAddress("glUseProgram");
    glDeleteProgram = (PFNGLDELETEPROGRAMPROC)glfwGetProcAddress("glDeleteProgram");
    glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)glfwGetProcAddress("glGetUniformLocation");
    glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)glfwGetProcAddress("glGetAttribLocation");
    glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)glfwGetProcAddress("glUniformMatrix4fv");
    glUniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC)glfwGetProcAddress("glUniformMatrix3fv");
    glUniform1f = (PFNGLUNIFORM1FPROC)glfwGetProcAddress("glUniform1f");
    glUniform1i = (PFNGLUNIFORM1IPROC)glfwGetProcAddress("glUniform1i");
    glUniform3f = (PFNGLUNIFORM3FPROC)glfwGetProcAddress("glUniform3f");
    glUniform4f = (PFNGLUNIFORM4FPROC)glfwGetProcAddress("glUniform4f");
    glActiveTexture = (PFNGLACTIVETEXTUREPROC)glfwGetProcAddress("glActiveTexture");

    glGenBuffers = (PFNGLGENBUFFERSPROC)glfwGetProcAddress("glGenBuffers");
    glBindBuffer = (PFNGLBINDBUFFERPROC)glfwGetProcAddress("glBindBuffer");
    glBufferData = (PFNGLBUFFERDATAPROC)glfwGetProcAddress("glBufferData");
    glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)glfwGetProcAddress("glDeleteBuffers");
    glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)glfwGetProcAddress("glEnableVertexAttribArray");
    glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)glfwGetProcAddress("glVertexAttribPointer");
    glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)glfwGetProcAddress("glDisableVertexAttribArray");
    loaded = 1;
    return 1;
}

#else

static inline int cgx_gl_load_extensions(void) {
    return 1;
}

#endif /* _WIN32 */

#endif /* CGX_GL_LOADER_H */
