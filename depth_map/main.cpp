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

int main()
{
    // Initialise GLFW
    if (!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); 
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //We don't want the old OpenGL 


    int width=1920;
    int height=1080;
    GLfloat near = 0.1f;
    GLfloat far = 300.0f;

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
        if( (pFile = fopen("./frame00003150.ply", "r+")) == NULL)
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

    glm::mat4 Projection, View;
    {
        // see http://ksimek.github.io/2013/06/03/calibrated_cameras_in_opengl/
        float fx = 1396.52f;
        float fy = 1393.52f;
        float cx = 933.738f;
        float cy = 560.443f;
        float projection_float[16] = {0.0f};
		float w = (float)width;
		float h = (float)height;

        Projection = glm::make_mat4(projection_float);
		Projection[0][0] = 2*fx/w;
		Projection[0][2] = (w-2*cx)/w;
		Projection[1][1] = 2*fy/h;
		Projection[1][2] = (2*cy-h)/h;
		Projection[2][2] = -(near+far)/(far-near);
		Projection[2][3] = -2*near*far/(far-near);
		Projection[3][2] = -1.0f;

		for(int i=0; i<4; i++){
			for(int j=0; j<4; j++){
				std::cout<<Projection[i][j]<<"\t";
			}
			std::cout<<std::endl;
		}


        float RT_float[16] = {
            -0.9225427896f,-0.01123881405f, -0.3857311115f, -10.70728522f,
            -0.02283482308f, 0.999414139f, 0.02549411087f, 143.7188498f,
            0.3852186031f, 0.03232750985f, -0.9222589441f, 277.0709018f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
        glm::mat4 RT = glm::make_mat4(RT_float);
        //View = glm::inverse(RT);
		View = glm::mat4(1.0f);

		for(int i=0; i<4; i++){
			for(int j=0; j<4; j++){
				std::cout<<View[i][j]<<"\t";
			}
			std::cout<<std::endl;
		}
    }

    // Projection matrix : 45бу Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
    // Projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, near, far);
    // 
    // // Camera matrix
    // View = glm::lookAt(
    //     glm::vec3(2, 2, 2), // Camera is at (4,3,3), in World Space
    //     glm::vec3(0, 0, 0), // and looks at the origin
    //     glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
    //     );

    // Model matrix : an identity matrix (model will be at the origin)
    glm::mat4 Model = glm::mat4(1.0f);
    // Our ModelViewProjection : multiplication of our 3 matrices
    glm::mat4 mvp = Projection * View * Model; // Remember, matrix multiplication is the other way around
	


    
    while (!glfwWindowShouldClose(window))
    {
        // Check and call events
        glfwPollEvents();

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
        glUniform1f(patchsizeID, 10.0f);

        // Use our shader
        glUseProgram(shaderProgram);

        // Draw the cube !
        glBindVertexArray(vao);

        //glDrawArrays(GL_TRIANGLES, 0, 12 * 3); 
        glDrawArrays(GL_POINTS, 0, g_vertex_buffer_data.size()/3); 
        glBindVertexArray(0);

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

    } 

    // Properly de-allocate all resources once they've outlived their purpose
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    // Terminate GLFW, clearing any resources allocated by GLFW.
    glfwTerminate();
    return 0;
}