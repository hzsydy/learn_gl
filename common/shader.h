#ifndef __SHADER_H__
#define __SHADER_H__

// Include standard headers
#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <random>

// Include GLEW. Always include it before gl.h and glfw.h, since it's a bit magic.
#include <GL/glew.h>
// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

//include OpenCV
#ifdef OPENCV_REQUIRED
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
GLuint LoadTexture2D(cv::Mat &img);
GLuint LoadTexture2D(const char * texture_image_path);
#endif

GLuint LoadShaders(const char * vertex_file_path, const char * fragment_file_path, const char* geometry_file_path = nullptr);


GLuint generateAttachmentTexture(GLboolean depth, GLboolean stencil, GLsizei screenWidth, GLsizei screenHeight);

#endif