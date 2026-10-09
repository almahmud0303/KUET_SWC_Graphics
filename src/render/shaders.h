#pragma once
#include "gl_loader.h"

namespace swc { namespace app {

extern const char* sceneVertexSource;
extern const char* sceneFragmentSource;
extern const char* depthVertexSource;
extern const char* depthFragmentSource;
extern const char* skyVertexSource;
extern const char* skyFragmentSource;
extern const char* imageFragmentSource;
GLuint compileShader(GLenum type,const char* source);
GLuint makeProgram(const char* vertexSource,const char* fragmentSource);

}} // namespace swc::app
