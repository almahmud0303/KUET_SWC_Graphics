#include "gl_loader.h"

void (APIENTRY *glGenVertexArrays)(GLsizei, GLuint *);
void (APIENTRY *glBindVertexArray)(GLuint);
void (APIENTRY *glDeleteVertexArrays)(GLsizei, const GLuint *);
void (APIENTRY *glGenBuffers)(GLsizei, GLuint *);
void (APIENTRY *glBindBuffer)(GLenum, GLuint);
void (APIENTRY *glBufferData)(GLenum, GLsizeiptr, const void *, GLenum);
void (APIENTRY *glDeleteBuffers)(GLsizei, const GLuint *);
GLuint (APIENTRY *glCreateShader)(GLenum);
void (APIENTRY *glShaderSource)(GLuint, GLsizei, const GLchar *const *, const GLint *);
void (APIENTRY *glCompileShader)(GLuint);
void (APIENTRY *glGetShaderiv)(GLuint, GLenum, GLint *);
void (APIENTRY *glGetShaderInfoLog)(GLuint, GLsizei, GLsizei *, GLchar *);
void (APIENTRY *glDeleteShader)(GLuint);
GLuint (APIENTRY *glCreateProgram)(void);
void (APIENTRY *glAttachShader)(GLuint, GLuint);
void (APIENTRY *glLinkProgram)(GLuint);
void (APIENTRY *glGetProgramiv)(GLuint, GLenum, GLint *);
void (APIENTRY *glGetProgramInfoLog)(GLuint, GLsizei, GLsizei *, GLchar *);
void (APIENTRY *glUseProgram)(GLuint);
void (APIENTRY *glDeleteProgram)(GLuint);
void (APIENTRY *glVertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void *);
void (APIENTRY *glEnableVertexAttribArray)(GLuint);
GLint (APIENTRY *glGetUniformLocation)(GLuint, const GLchar *);
void (APIENTRY *glUniformMatrix4fv)(GLint, GLsizei, GLboolean, const GLfloat *);
void (APIENTRY *glUniform3f)(GLint, GLfloat, GLfloat, GLfloat);
void (APIENTRY *glUniformMatrix3fv)(GLint, GLsizei, GLboolean, const GLfloat *);
void (APIENTRY *glUniform1i)(GLint, GLint);
void (APIENTRY *glGenFramebuffers)(GLsizei, GLuint *);
void (APIENTRY *glBindFramebuffer)(GLenum, GLuint);
void (APIENTRY *glFramebufferTexture2D)(GLenum, GLenum, GLenum, GLuint, GLint);
GLenum (APIENTRY *glCheckFramebufferStatus)(GLenum);
void (APIENTRY *glDeleteFramebuffers)(GLsizei, const GLuint *);

#define LOAD(name) do { *(void **)(&name) = get_proc_address(#name); if (!(name)) return 0; } while (0)

int load_opengl_functions(void *(*get_proc_address)(const char *)) {
    LOAD(glGenVertexArrays);
    LOAD(glBindVertexArray);
    LOAD(glDeleteVertexArrays);
    LOAD(glGenBuffers);
    LOAD(glBindBuffer);
    LOAD(glBufferData);
    LOAD(glDeleteBuffers);
    LOAD(glCreateShader);
    LOAD(glShaderSource);
    LOAD(glCompileShader);
    LOAD(glGetShaderiv);
    LOAD(glGetShaderInfoLog);
    LOAD(glDeleteShader);
    LOAD(glCreateProgram);
    LOAD(glAttachShader);
    LOAD(glLinkProgram);
    LOAD(glGetProgramiv);
    LOAD(glGetProgramInfoLog);
    LOAD(glUseProgram);
    LOAD(glDeleteProgram);
    LOAD(glVertexAttribPointer);
    LOAD(glEnableVertexAttribArray);
    LOAD(glGetUniformLocation);
    LOAD(glUniformMatrix4fv);
    LOAD(glUniform3f);
    LOAD(glUniformMatrix3fv);
    LOAD(glUniform1i);
    LOAD(glGenFramebuffers);
    LOAD(glBindFramebuffer);
    LOAD(glFramebufferTexture2D);
    LOAD(glCheckFramebufferStatus);
    LOAD(glDeleteFramebuffers);
    return 1;
}
