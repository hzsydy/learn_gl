#define OPENCV_REQUIRED
#include "../common/shader.h"

// Include GLEW. Always include it before gl.h and glfw.h, since it's a bit magic.
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>

#include <glm/gtc/type_ptr.hpp>

#include <thread>
#include <fstream>
#include <sstream>
#include <iterator>
#include <algorithm>

//static GLfloat g_vertex_buffer_data[] = {};
std::vector<GLfloat> g_vertex_buffer_data;

std::random_device rd;
std::mt19937 rng(rd());

float gen_random_float(float min=0.0f, float max=1.0f)
{
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

int main(int argc, char** argv)
{
	if (argc != 3)
	{
		std::cout<<"usage: depth_map *.ply *.depth";
		exit(-1);
	}

    // Initialise GLFW
    if (!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW\n");
		exit(-1);
    }

    glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); 
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //We don't want the old OpenGL 


    int width=1920;
    int height=1080;
    GLfloat near = 0.1f;
    GLfloat far = 1000.0f;

    // Open a window and create its OpenGL context
    GLFWwindow* window; // (In the accompanying source code, this variable is global)
    window = glfwCreateWindow(width, height, "depth", NULL, NULL);
    if (window == NULL) {
        fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window); // Initialize GLEW

    // Set the required callback functions
    glfwSetKeyCallback(window, key_callback);

    glewExperimental = true; // Needed in core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return -1;
    }

    // Define the viewport dimensions
    glfwGetFramebufferSize(window, &width, &height);  
    glViewport(0, 0, width, height);

    // Ensure we can capture the escape key being pressed below
    // glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);


    //read ply
    {
        FILE *pFile;
        if( (pFile = fopen(argv[1], "r+")) == NULL)
        {
            printf("No such file\n");
            exit(1);
        }
        char line[256];
        int num_vertex;
        float x,y,z;

        fgets(line, sizeof(line), pFile);
        fgets(line, sizeof(line), pFile);
        fgets(line, sizeof(line), pFile);
        fgets(line, sizeof(line), pFile);
        fscanf(pFile, "element vertex %d\n", &num_vertex);
        for (int i=0; i<9; i++)
        {
            fgets(line, sizeof(line), pFile);
        }
        for (int i=0; i<num_vertex; i++)
        {
            fscanf(pFile, "%f %f %f\n", &x, &y, &z);
            if (y<-5.0)
            {
                g_vertex_buffer_data.push_back(x);
                g_vertex_buffer_data.push_back(y);
                g_vertex_buffer_data.push_back(z);
            }
        }
        fclose(pFile);
    }

	//{
    //    FILE *pFile;
	//	if( (pFile = fopen("./frame00003150.xyz", "w")) == NULL)
    //    {
    //        printf("gg\n");
    //        exit(1);
    //    }
	//	for (int i = 0; i < g_vertex_buffer_data.size()/3; i++)
	//	{
	//		fprintf(pFile, "%f %f %f\n", g_vertex_buffer_data[3*i], g_vertex_buffer_data[3*i+1], g_vertex_buffer_data[3*i+2]);
	//
	//	}
    //    fclose(pFile);
	//}


    // Create and compile our GLSL program from the shaders
    GLuint shaderProgram = LoadShaders("./shaders/depth.vert", "./shaders/depth.frag", "./shaders/depth.geo");

    //vao
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);


    //vbo
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*g_vertex_buffer_data.size(), &(g_vertex_buffer_data[0]), GL_STATIC_DRAW);

    // 1rst attribute buffer : vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(
        0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
        3,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        0,                  // stride
        (void*)0            // array buffer offset
        );

    // Note that this is allowed, the call to glVertexAttribPointer registered VBO as the currently bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0); 

    // Unbind VAO (it's always a good thing to unbind any buffer/array to prevent strange bugs)
    glBindVertexArray(0); 


	//////THE OFFLINE RENDERING
	// create a texture object
	GLuint textureId;
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE); // automatic mipmap
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0,
				 GL_RGBA, GL_FLOAT, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	// create a renderbuffer object to store depth info
	GLuint rbo;
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT,
						   width, height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	// create a framebuffer object
	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	// attach the texture to FBO color attachment point
	glFramebufferTexture2D(GL_FRAMEBUFFER,        // 1. fbo target: GL_FRAMEBUFFER 
						   GL_COLOR_ATTACHMENT0,  // 2. attachment point
						   GL_TEXTURE_2D,         // 3. tex target: GL_TEXTURE_2D
						   textureId,             // 4. tex ID
						   0);                    // 5. mipmap level: 0(base)

	// attach the renderbuffer to depth attachment point
	glFramebufferRenderbuffer(GL_FRAMEBUFFER,      // 1. fbo target: GL_FRAMEBUFFER
							  GL_DEPTH_ATTACHMENT, // 2. attachment point
							  GL_RENDERBUFFER,     // 3. rbo target: GL_RENDERBUFFER
							  rbo);              // 4. rbo ID

	// check FBO status
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(status != GL_FRAMEBUFFER_COMPLETE)
		return -1;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glm::mat4 Projection, View;
    {
        float fx = 1396.52f;
        float fy = 1393.52f;
        float cx = 933.738f;
        float cy = 560.443f;
		float w = (float)width;
		float h = (float)height;

        float l = 0.0, r = 1.0*w, b = 1.0*h, t = 0.0;
        float tx = -(r+l)/(r-l), ty = -(t+b)/(t-b), tz = -(far+near)/(far-near);
        float ortho_float[16] = {2.0/(r-l), 0.0, 0.0, tx,
            0.0, 2.0/(t-b), 0.0, ty,
            0.0, 0.0, -2.0/(far-near), tz,
            0.0, 0.0, 0.0, 1.0};
        float Intrinsic_float[16] = {fx, 0, cx, 0.0,
            0.0, fy, cy, 0.0,
            0.0, 0.0, -(near+far), +near*far,
            0.0, 0.0, 1.0 , 0.0};
        glm::mat4 ortho = glm::make_mat4(ortho_float);
        glm::mat4 Intrinsic = glm::make_mat4(Intrinsic_float);
        
        ortho = glm::transpose(ortho);
        Intrinsic = glm::transpose(Intrinsic);

        Projection = ortho*Intrinsic;

        float RT_float[16] = {
            -0.9225427896f,-0.01123881405f, -0.3857311115f, -10.70728522f,
            -0.02283482308f, 0.999414139f, 0.02549411087f, 143.7188498f,
            0.3852186031f, 0.03232750985f, -0.9222589441f, 277.0709018f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
        glm::mat4 RT = glm::transpose(glm::make_mat4(RT_float));
        View = RT;
        //View = glm::inverse(RT);
		//View = glm::mat4(1.0f);

    }

    // Model matrix : an identity matrix (model will be at the origin)
    glm::mat4 Model = glm::mat4(1.0f);
    // Our ModelViewProjection : multiplication of our 3 matrices
    glm::mat4 mvp = Projection * View * Model; // Remember, matrix multiplication is the other way around
	
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);

    
    for (int i=0; i<3; i++)
    {
        // Check and call events
        glfwPollEvents();
	

		//glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		//glViewport(0,0,width,height); // Render on the whole framebuffer, complete from the lower left corner to the upper right

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Get a handle for our "MVP" uniform
        // Only during the initialisation
        GLuint MatrixID = glGetUniformLocation(shaderProgram, "MVP");
        GLuint nearID = glGetUniformLocation(shaderProgram, "near");
        GLuint farID = glGetUniformLocation(shaderProgram, "far");
        GLuint patchsizeID = glGetUniformLocation(shaderProgram, "patchsize");

        // Send our transformation to the currently bound shader, in the "MVP" uniform
        // This is done in the main loop since each model will have a different MVP matrix (At least for the M part)
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &mvp[0][0]);
        glUniform1f(nearID, near);
        glUniform1f(farID, far);
        glUniform1f(patchsizeID, 0.8f);

        // Use our shader
        glUseProgram(shaderProgram);

        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, g_vertex_buffer_data.size()); 
        glBindVertexArray(0);
		//{
		//	FILE *pFile;
		//	if( (pFile = fopen(argv[2], "wb")) == NULL)
		//	{
		//		printf("gg\n");
		//		exit(1);
		//	}
		//	fwrite(rgbChannels[0].data, sizeof(float), height*width, pFile);
		//	fclose(pFile);
		//}
		
		//glBindFramebuffer(GL_FRAMEBUFFER, 0);
		

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
	}
     
	cv::Mat screen(height, width, CV_32FC3);
	glReadPixels(0, 0, width, height, GL_BGR_EXT, GL_FLOAT, screen.data);
	cv::flip(screen, screen, 0);
	std::vector<cv::Mat> rgbChannels(3);
	cv::split(screen, rgbChannels);
	cv::Mat save_img;
	rgbChannels[0].convertTo(save_img, CV_16UC1, 10000.0);
	//cv::imwrite("fuck.png", save_img);
	//rgbChannels[0].convertTo(save_img, CV_8UC1, 1000.0);
	cv::imshow("", save_img);
	cv::waitKey(1);
	cv::imwrite("fuck.png",save_img);

    // Properly de-allocate all resources once they've outlived their purpose
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    // Terminate GLFW, clearing any resources allocated by GLFW.
    glfwTerminate();
    return 0;
}