#include "../common/shader.h"

// Include GLEW. Always include it before gl.h and glfw.h, since it's a bit magic.
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>

#include <thread>

std::random_device rd;
std::mt19937 rng(rd());

float gen_random_float(float min=0.0f, float max=1.0f)
{
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
}

/**********************
* CAMERA MOVE WITH KEYBOARD
***********************/
bool keys[1024];  
glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f,  3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
GLfloat deltaTime = 0.0f;	// Time between current frame and last frame
GLfloat lastFrame = 0.0f;  	// Time of last frame

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
    if(action == GLFW_PRESS)
        keys[key] = true;
    else if(action == GLFW_RELEASE)
        keys[key] = false;  
}

GLfloat lastX = 400, lastY = 300;
bool firstMouse = true;
GLfloat roll = 0, yaw = 0, pitch = 0;

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if(firstMouse)
    {
        lastX = (GLfloat)xpos;
        lastY = (GLfloat)ypos;
        firstMouse = false;
    }
    else
    {
        GLfloat xoffset = (GLfloat)xpos - lastX;
        GLfloat yoffset = lastY - (GLfloat)ypos; 
        lastX = (GLfloat)xpos;
        lastY = (GLfloat)ypos;

        GLfloat sensitivity = 0.01f;
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        yaw   += xoffset;
        pitch += yoffset;

        if(pitch > 89.0f)
            pitch = 89.0f;
        if(pitch < -89.0f)
            pitch = -89.0f;

        glm::vec3 front;
        //front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        //front.y = sin(glm::radians(pitch));
        //front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.x = sin(glm::radians(yaw));
        front.y = sin(glm::radians(pitch)) * cos(glm::radians(yaw));
        front.z = -1.0f*cos(glm::radians(yaw)) * cos(glm::radians(pitch));

        cameraFront = glm::normalize(front);
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    GLfloat scrollSpeed = 0.05f;
    cameraPos += (GLfloat)yoffset * scrollSpeed * cameraFront;
}

void do_movement()
{
    GLfloat currentFrame = (GLfloat)glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;  
    // Camera controls
    GLfloat cameraSpeed = 5.0f * deltaTime;
    if(keys[GLFW_KEY_W])
        cameraPos += glm::normalize(glm::cross(cameraFront, glm::vec3(1.0f, 0.0f, 0.0f))) * cameraSpeed;
    if(keys[GLFW_KEY_S])
        cameraPos -= glm::normalize(glm::cross(cameraFront, glm::vec3(1.0f, 0.0f, 0.0f))) * cameraSpeed;
    if(keys[GLFW_KEY_A])
        cameraPos += glm::normalize(glm::cross(cameraFront, glm::vec3(0.0f, 1.0f, 0.0f))) * cameraSpeed;
    if(keys[GLFW_KEY_D])
        cameraPos -= glm::normalize(glm::cross(cameraFront, glm::vec3(0.0f, 1.0f, 0.0f))) * cameraSpeed;
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
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //We don't want the old OpenGL 

    // Open a window and create its OpenGL context
    GLFWwindow* window; // (In the accompanying source code, this variable is global)
    window = glfwCreateWindow(1024, 768, "Tutorial 02", NULL, NULL);
    if (window == NULL) {
        fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window); // Initialize GLEW

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); 
    // Set the required callback functions
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback); 
    glfwSetScrollCallback(window, scroll_callback); 

    glewExperimental = true; // Needed in core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return -1;
    }

    // Define the viewport dimensions
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);  
    glViewport(0, 0, width, height);

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);


    GLfloat vertices[] = {
        // Positions          // Colors           // Texture Coords
        0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // Top Right
        0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // Bottom Right
        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // Bottom Left
        -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // Top Left 
    };

    GLuint indices[] = {  // Note that we start from 0!
        0, 1, 3, // First Triangle
        1, 2, 3  // Second Triangle
    };

    //vao
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    //vbo
    GLuint vbo;
    GLuint ebo;
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,                                 // attribute 0. No particular reason for 0, but must match the layout in the shader.
        3,                                 // size
        GL_FLOAT,                          // type
        GL_FALSE,                          // normalized?
        8 * sizeof(GLfloat),               // stride
        (GLvoid*)0                         // array buffer offset
        );
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,                  
        3,                  
        GL_FLOAT,           
        GL_FALSE,           
        8 * sizeof(GLfloat),
        (GLvoid*)(3 * sizeof(GLfloat))           
        );
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2,                  
        2,                  
        GL_FLOAT,           
        GL_FALSE,           
        8 * sizeof(GLfloat),
        (GLvoid*)(6 * sizeof(GLfloat))            
        );

    // Note that this is allowed, the call to glVertexAttribPointer registered VBO as the currently bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0); 

    // Unbind VAO (it's always a good thing to unbind any buffer/array to prevent strange bugs)
    glBindVertexArray(0); 

    // Create and compile our GLSL program from the shaders
    GLuint shaderProgram = LoadShaders("./shaders/TextureVertexShader.glsl", "./shaders/TextureFragmentShader.glsl");
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");

    GLuint textureWall = LoadTexture2D("./wall.jpg");
    GLuint textureBleed = LoadTexture2D("./bleed.png");

    int c=0;

    while (!glfwWindowShouldClose(window))
    {
        // Check and call events
        glfwPollEvents();
        do_movement();  

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view;
        view = glm::lookAt(cameraPos, cameraPos + cameraFront, glm::vec3(0.0f, 1.0f, 0.0f));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        glm::mat4 projection;
        projection = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.0f);
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Use our shader
        glUseProgram(shaderProgram);

        // Draw 
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureWall);
        glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textureBleed);
        glUniform1i(glGetUniformLocation(shaderProgram, "texture2"), 1);

        glBindVertexArray(vao);

        glm::vec3 cubePositions[] = {
            glm::vec3( 0.0f,  0.0f,  0.0f), 
            glm::vec3( 2.0f,  5.0f, -15.0f), 
            glm::vec3(-1.5f, -2.2f, -2.5f),  
            glm::vec3(-3.8f, -2.0f, -12.3f),  
            glm::vec3( 2.4f, -0.4f, -3.5f),  
            glm::vec3(-1.7f,  3.0f, -7.5f),  
            glm::vec3( 1.3f, -2.0f, -2.5f),  
            glm::vec3( 1.5f,  2.0f, -2.5f), 
            glm::vec3( 1.5f,  0.2f, -1.5f), 
            glm::vec3(-1.3f,  1.0f, -1.5f)  
        };
        for(GLuint i = 0; i < 10; i++)
        {
            glm::mat4 model;
            model = glm::translate(model, cubePositions[i]);
            GLfloat angle = 20.0f * i; 
            model = glm::rotate(model, angle, glm::vec3(1.0f, 0.3f, 0.5f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }
        //glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glBindVertexArray(0);

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();

        //std::this_thread::sleep_for(std::chrono::milliseconds(500));
    } 

    // Properly de-allocate all resources once they've outlived their purpose
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    // Terminate GLFW, clearing any resources allocated by GLFW.
    glfwTerminate();
    return 0;
}