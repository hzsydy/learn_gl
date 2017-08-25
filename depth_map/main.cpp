#define OPENCV_REQUIRED
#include "../common/shader.h"

// Include GLEW. Always include it before gl.h and glfw.h, since it's a bit magic.
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>

#include <glm/gtc/type_ptr.hpp>

#include <json/json.h>

#include "../3rdparty/rply-1.1.4/rply.h"

#include <thread>
#include <fstream>
#include <sstream>
#include <iterator>
#include <algorithm>

std::random_device rd;
std::mt19937 rng(rd());

const float scale = 0.2f;

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

std::vector<GLfloat> g_vertex_buffer_data;
float ply_buf[3];
int ply_buf_c=0;

static int vertex_cb(p_ply_argument argument) {
    long eol;
    ply_get_argument_user_data(argument, NULL, &eol);
    //printf("%g", ply_get_argument_value(argument));
    ply_buf[ply_buf_c] = (float)ply_get_argument_value(argument);
    ply_buf_c++;
    if (ply_buf_c==3) {
        ply_buf_c = 0;
        if (ply_buf[1]<-5.0 && (ply_buf[0]*ply_buf[0]+ply_buf[2]*ply_buf[2])<45000)
        {
            g_vertex_buffer_data.push_back(ply_buf[0]);
            g_vertex_buffer_data.push_back(ply_buf[1]);
            g_vertex_buffer_data.push_back(ply_buf[2]);
        }
    }
    return 1;
}

int main(int argc, char** argv)
{
    if (argc != 6)
    {
        std::cout<<"usage: depth_map *.ply *.depth calib.json 0 5";
        exit(-1);
    }
    const char* calib_file = argv[3];
    int panel_number = std::atoi(argv[4]);
    int camera_number = std::atoi(argv[5]);
    char camera_name[256] = {};
    sprintf(camera_name, "%02d_%02d", panel_number, camera_number);

    //read plys
    {
        long nvertices, ntriangles;
        p_ply ply = ply_open(argv[1], NULL, 0, NULL);
        if (!ply) return 1;
        if (!ply_read_header(ply)) return 1;
        nvertices = ply_set_read_cb(ply, "vertex", "x", vertex_cb, NULL, 0);
        ply_set_read_cb(ply, "vertex", "y", vertex_cb, NULL, 0);
        ply_set_read_cb(ply, "vertex", "z", vertex_cb, NULL, 1);
        ntriangles = ply_set_read_cb(ply, "face", "vertex_indices", NULL, NULL, 0);
        //printf("%ld\n%ld\n", nvertices, ntriangles);
        if (!ply_read(ply)) return 1;
        ply_close(ply);
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
    glfwWindowHint(GLFW_VISIBLE, GL_FALSE);

    int width=(int)(1920*scale);
    int height=(int)(1080*scale);
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
    // glfwSetKeyCallback(window, key_callback);

    glewExperimental = true; // Needed in core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return -1;
    }


    // Ensure we can capture the escape key being pressed below
    // glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);


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
    GLuint offline_tex;
    glGenTextures(1, &offline_tex);
    glBindTexture(GL_TEXTURE_2D, offline_tex);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE); // automatic mipmap
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0,
        GL_RGBA, GL_FLOAT, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // create a framebuffer object
    GLuint fbo;
    glGenFramebuffersEXT(1, &fbo);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);

    // create a renderbuffer object to store depth info
    GLuint rbo;
    glGenRenderbuffersEXT(1, &rbo);
    glBindRenderbufferEXT(GL_RENDERBUFFER, rbo);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24,
        width, height);


    // attach the texture to FBO color attachment point
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,        // 1. fbo target: GL_FRAMEBUFFER 
        GL_COLOR_ATTACHMENT0_EXT,  // 2. attachment point
        GL_TEXTURE_2D,         // 3. tex target: GL_TEXTURE_2D
        offline_tex,             // 4. tex ID
        0);                    // 5. mipmap level: 0(base)

    // attach the renderbuffer to depth attachment point
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,      // 1. fbo target: GL_FRAMEBUFFER
        GL_DEPTH_ATTACHMENT, // 2. attachment point
        GL_RENDERBUFFER,     // 3. rbo target: GL_RENDERBUFFER
        rbo);              // 4. rbo ID

    // check FBO status
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE_EXT)
    {
        std::cout <<"cannot create fbo ext";
        exit(-1);
    }
    glBindRenderbufferEXT(GL_RENDERBUFFER, 0);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

    // Define the viewport dimensions
    glViewport(0, 0, width, height);

    glm::mat4 Projection, View;
    {
        Json::Value root;
        Json::Reader reader;

        std::ifstream file(calib_file, std::ifstream::binary);
        if(!reader.parse(file, root, true)){
            std::cout  << "Failed to parse configuration\n"
                << reader.getFormattedErrorMessages();
        }

        const Json::Value cameras = root["cameras"];
        Json::Value camera;
        for (int i = 0; i < cameras.size(); i++)
        {
            if (cameras[i]["name"].asString() == camera_name)
            {
                camera = cameras[i];
                break;
            }
        }
        float fx = camera["K"][0][0].asFloat()*scale;
        float fy = camera["K"][1][1].asFloat()*scale;
        float cx = camera["K"][0][2].asFloat()*scale;
        float cy = camera["K"][1][2].asFloat()*scale;
        float RT_float[16] = {0.0f};
        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                RT_float[i*4+j] = camera["R"][i][j].asFloat();
            }
            RT_float[i*4+3] = camera["t"][i][0].asFloat();
        }
        RT_float[15] = 1.0f;


        //float fx = 1396.52f;
        //float fy = 1393.52f;
        //float cx = 933.738f;
        //float cy = 560.443f;
        //
        //float RT_float[16] = {
        //    -0.9225427896f,-0.01123881405f, -0.3857311115f, -10.70728522f,
        //    -0.02283482308f, 0.999414139f, 0.02549411087f, 143.7188498f,
        //    0.3852186031f, 0.03232750985f, -0.9222589441f, 277.0709018f,
        //    0.0f, 0.0f, 0.0f, 1.0f
        //};

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

        glm::mat4 RT = glm::transpose(glm::make_mat4(RT_float));
        View = RT;
    }

    // Model matrix : an identity matrix (model will be at the origin)
    glm::mat4 Model = glm::mat4(1.0f);
    // Our ModelViewProjection : multiplication of our 3 matrices
    glm::mat4 mvp = Projection * View * Model; // Remember, matrix multiplication is the other way around


    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
    

    // Create and compile our GLSL program from the shaders
    GLuint shaderProgram = LoadShaders("./shaders/depth.vert", "./shaders/depth.frag", "./shaders/depth.geo");

    for (int i=0; i<2; i++)
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
        glUniform1f(patchsizeID, 0.8f);

        // Use our shader
        glUseProgram(shaderProgram);

        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, g_vertex_buffer_data.size()); 
        glBindVertexArray(0);

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    cv::Mat screen(height, width, CV_32FC3);
    std::vector<cv::Mat> rgbChannels(3);
    cv::Mat save_img_densified, save_img_non_densified;

    glReadPixels(0, 0, width, height, GL_BGR_EXT, GL_FLOAT, screen.data);
    cv::flip(screen, screen, 0);
    cv::split(screen, rgbChannels);
    rgbChannels[0].convertTo(save_img_densified, CV_16UC1, 10000.0);
    cv::imwrite(argv[2],save_img_densified);

    /*
    glEnable(GL_PROGRAM_POINT_SIZE);
    GLuint shaderProgram_nopatch = LoadShaders("./shaders/depth_nopatch.vert", "./shaders/depth.frag", "./shaders/depth_nopatch.geo");

    for (int i=0; i<3; i++)
    {
        // Check and call events
        glfwPollEvents();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Get a handle for our "MVP" uniform
        // Only during the initialisation
        GLuint MatrixID = glGetUniformLocation(shaderProgram_nopatch, "MVP");
        GLuint nearID = glGetUniformLocation(shaderProgram_nopatch, "near");
        GLuint farID = glGetUniformLocation(shaderProgram_nopatch, "far");

        // Send our transformation to the currently bound shader, in the "MVP" uniform
        // This is done in the main loop since each model will have a different MVP matrix (At least for the M part)
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &mvp[0][0]);
        glUniform1f(nearID, near);
        glUniform1f(farID, far);

        // Use our shader
        glUseProgram(shaderProgram_nopatch);

        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, g_vertex_buffer_data.size()); 
        glBindVertexArray(0);

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glReadPixels(0, 0, width, height, GL_BGR_EXT, GL_FLOAT, screen.data);
    cv::flip(screen, screen, 0);
    cv::split(screen, rgbChannels);
    rgbChannels[0].convertTo(save_img_non_densified, CV_16UC1, 10000.0);
    cv::imwrite("fuck.png",save_img_non_densified);
    */

    // Properly de-allocate all resources once they've outlived their purpose
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    //Delete resources
    glDeleteTextures(1, &offline_tex);
    glDeleteRenderbuffersEXT(1, &rbo);
    //Bind 0, which means render to back buffer, as a result, fb is unbound
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    glDeleteFramebuffersEXT(1, &fbo);
    // Terminate GLFW, clearing any resources allocated by GLFW.
    glfwTerminate();
    return 0;
}