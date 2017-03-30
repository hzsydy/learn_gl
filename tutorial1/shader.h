#ifndef __SHADER_H__
#define __SHADER_H__

// Include standard headers
#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>

// Include GLEW. Always include it before gl.h and glfw.h, since it's a bit magic.
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
using namespace glm;

GLuint LoadShaders(const char * vertex_file_path, const char * fragment_file_path);

#endif