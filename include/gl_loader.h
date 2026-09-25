#ifndef GL_LOADER_H
#define GL_LOADER_H

#include <stddef.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <GL/gl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;

#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#define GL_STATIC_DRAW 0x88E4
#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#endif

extern void (APIENTRY *glGenVertexArrays)(GLsizei, GLuint *);
extern void (APIENTRY *glBindVertexArray)(GLuint);
extern void (APIENTRY *glDeleteVertexArrays)(GLsizei, const GLuint *);
extern void (APIENTRY *glGenBuffers)(GLsizei, GLuint *);
extern void (APIENTRY *glBindBuffer)(GLenum, GLuint);
extern void (APIENTRY *glBufferData)(GLenum, GLsizeiptr, const void *, GLenum);
extern void (APIENTRY *glDeleteBuffers)(GLsizei, const GLuint *);
extern GLuint (APIENTRY *glCreateShader)(GLenum);
extern void (APIENTRY *glShaderSource)(GLuint, GLsizei, const GLchar *const *, const GLint *);
extern void (APIENTRY *glCompileShader)(GLuint);
extern void (APIENTRY *glGetShaderiv)(GLuint, GLenum, GLint *);
extern void (APIENTRY *glGetShaderInfoLog)(GLuint, GLsizei, GLsizei *, GLchar *);
extern void (APIENTRY *glDeleteShader)(GLuint);
extern GLuint (APIENTRY *glCreateProgram)(void);
extern void (APIENTRY *glAttachShader)(GLuint, GLuint);
extern void (APIENTRY *glLinkProgram)(GLuint);
extern void (APIENTRY *glGetProgramiv)(GLuint, GLenum, GLint *);
extern void (APIENTRY *glGetProgramInfoLog)(GLuint, GLsizei, GLsizei *, GLchar *);
extern void (APIENTRY *glUseProgram)(GLuint);
extern void (APIENTRY *glDeleteProgram)(GLuint);
extern void (APIENTRY *glVertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void *);
extern void (APIENTRY *glEnableVertexAttribArray)(GLuint);
extern GLint (APIENTRY *glGetUniformLocation)(GLuint, const GLchar *);
extern void (APIENTRY *glUniformMatrix4fv)(GLint, GLsizei, GLboolean, const GLfloat *);
extern void (APIENTRY *glUniform3f)(GLint, GLfloat, GLfloat, GLfloat);

int load_opengl_functions(void *(*get_proc_address)(const char *));

#ifdef __cplusplus
}
#endif

#endif

