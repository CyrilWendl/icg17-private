// glew must be before glfw
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// contains helper functions such as shader compiler
#include "icg_helper.h"

#include <glm/gtc/matrix_transform.hpp>
#include "grid/grid.h"
#include "quad/quad.h"
#include "water/water.h"
#include "trackball.h"
#include "skybox/skybox.h"
#include "framebuffer.h"

int window_width = 800;
int window_height = 600;

using namespace glm;

mat4 projection_matrix;
mat4 view_matrix;
mat4 trackball_matrix;
mat4 old_trackball_matrix;
mat4 quad_model_matrix;
float prev_y = 0.0f;
Trackball trackball;

// Camera
glm::vec3 cameraPos    = vec3(0.0f, 0.5f, 2.0f);
glm::vec3 cameraFront = vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp     = vec3(0.0f, 1.0f, 0.0f);
GLfloat yaw_cam   = -90.0f;	// Yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right (due to how Eular angles work) so we initially rotate a bit to the left.
GLfloat pitch_cam =   0.0f;
GLfloat lastX =  window_width  / 2.0;
GLfloat lastY =  window_height / 2.0;
GLfloat fov =  45.0f;
bool keys[1024];

// Deltatime
GLfloat deltaTime = 0.0f;	// Time between current frame and last frame
GLfloat lastFrame = 0.0f;  	// Time of last frame

FrameBuffer framebuffer;
Grid grid;
Quad quad;
Water water;
Skybox skybox;

float H = 0.1;// we never use these two variables
float lacunarity = 0.1;

int octaves = 5;
float amplitude = .7f;
float frequency = 2.7f;


mat4 PerspectiveProjection(float left, float right, float bottom,
                           float top, float near, float far) {
    assert(right > left);
    assert(-far > -near);
    assert(top > bottom);
    mat4 projection = mat4(1.0f);

    projection[0][0] = 2.0f*near / (right - left);
    projection[1][1] = 2.0f*near / (top - bottom);
    projection[2][0] = (right+left) / (right-left);
    projection[2][1] = (top+bottom) / (top - bottom);
    projection[2][2] = -(far+near) / (far - near);
    projection[2][3] = -1.0f;
    projection[3][2] = -(2.0f*far*near) / (far - near);
    projection[3][3] = 0.0f;
    return projection;
}

// transforms glfw screen coordinates into normalized OpenGL coordinates.
vec2 TransformScreenCoords(GLFWwindow* window, int x, int y) {
    // the framebuffer and the window doesn't necessarily have the same size
    // i.e. hidpi screens. so we need to get the correct one
    int width;
    int height;
    glfwGetWindowSize(window, &width, &height);
    return vec2(2.0f * (float)x / width - 1.0f,
                1.0f - 2.0f * (float)y / height);
}

void MouseButton(GLFWwindow* window, int button, int action, int mod) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double x_i, y_i;
        glfwGetCursorPos(window, &x_i, &y_i);
        vec2 p = TransformScreenCoords(window, x_i, y_i);
        trackball.BeingDrag(p.x, p.y);
        old_trackball_matrix = trackball_matrix;
        // Store the current state of the model matrix.
    }
}

void MousePos(GLFWwindow* window, double x, double y) {
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        vec2 p = TransformScreenCoords(window, x, y);
        trackball_matrix =  old_trackball_matrix * trackball.Drag(p.x, p.y);
        grid.light_pos = vec3(-p.x, -p.y, 2.0f);
    }

    // zoom
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        vec2 p = TransformScreenCoords(window, x, y);
        float z_factor = 0.0f;
        if(p.y > prev_y) {
            z_factor = 0.1f;
        } else if(p.y < prev_y) {
            z_factor = -0.1f;
        } else {
            z_factor = 0.0f;
        }

        prev_y = p.y;
        view_matrix = translate(view_matrix, vec3(0, 0, z_factor));
    }
}

// Gets called when the windows/framebuffer is resized.
void SetupProjection(GLFWwindow* window, int width, int height) {
    window_width = width;
    window_height = height;

    cout << "Window has been resized to "
         << window_width << "x" << window_height << "." << endl;

    glViewport(0, 0, window_width, window_height);

    GLfloat top = 0.2f;
    GLfloat right = (GLfloat)window_width / window_height * top;
    projection_matrix = PerspectiveProjection(-right, right, -top, top, 0.5f, -0.5f);
}

void ErrorCallback(int error, const char* description) {
    fputs(description, stderr);
}

void Init(GLFWwindow* window) {
    // sets background color
    glClearColor(0.937, 0.937, 0.937 /*gray*/, 1.0 /*solid*/);


    // enable depth test.
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    //glEnable(GL_BLEND); // for transparency
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // initialize framebuffer
    glfwGetFramebufferSize(window, &window_width, &window_height);
    GLuint framebuffer_texture_id = framebuffer.Init(window_width, window_height);
    // initialize the quad with the framebuffer calculated perlin noise texture
    grid.Init(framebuffer_texture_id);
    skybox.Init();
    water.Init();
    quad.Init();

}

// gets called for every frame.
void Display() {
    glViewport(0,0,window_width,window_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const float time = glfwGetTime();

    framebuffer.Bind();
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        quad.Draw(projection_matrix * view_matrix * trackball_matrix * quad_model_matrix,octaves,amplitude,frequency);
    }
    framebuffer.Unbind();
    glViewport(0,0,window_width,window_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glm::mat4 view = glm::mat4(glm::mat3(view_matrix));
    skybox.Draw(projection_matrix * view * trackball_matrix * quad_model_matrix);
    grid.Draw(time, trackball_matrix * quad_model_matrix, view_matrix, projection_matrix);
    water.Draw(time, trackball_matrix * quad_model_matrix, view_matrix, projection_matrix);

}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
    if (key >= 0 && key < 1024)
    {
        if (action == GLFW_PRESS)
            keys[key] = true;
        else if (action == GLFW_RELEASE)
            keys[key] = false;
    }

    // Terrain
    if (keys[GLFW_KEY_P])
        octaves+=1;
    if (keys[GLFW_KEY_O])
        octaves-=1;
    if (keys[GLFW_KEY_UP])
        amplitude+=.1;
    if (keys[GLFW_KEY_DOWN])
        amplitude-=.1;
    if (keys[GLFW_KEY_RIGHT])
        frequency+=.1;
    if (keys[GLFW_KEY_LEFT])
        frequency-=.1;
        cout << "Frequency :" << frequency << endl;
        cout << "Amplitude: " << amplitude << endl;
        cout << "Octaves: " << octaves << endl;
}

void do_movement()
{
    // Camera controls
    GLfloat cameraSpeed = 5.0f * deltaTime;
    if (keys[GLFW_KEY_W])
        cameraPos += cameraSpeed * cameraFront;
    if (keys[GLFW_KEY_S])
        cameraPos -= cameraSpeed * cameraFront;
    if (keys[GLFW_KEY_A])
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (keys[GLFW_KEY_D])
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

}

bool firstMouse = true;
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    GLfloat xoffset = xpos - lastX;
    GLfloat yoffset = lastY - ypos; // Reversed since y-coordinates go from bottom to left
    lastX = xpos;
    lastY = ypos;

    GLfloat sensitivity = 0.05;	// Change this value to your liking
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw_cam += xoffset;
    pitch_cam += yoffset;

    // Make sure that when pitch is out of bounds, screen doesn't get flipped
    if (pitch_cam > 89.0f)
        pitch_cam = 89.0f;
    if (pitch_cam < -89.0f)
        pitch_cam = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw_cam)) * cos(glm::radians(pitch_cam));
    front.y = sin(glm::radians(pitch_cam));
    front.z = sin(glm::radians(yaw_cam)) * cos(glm::radians(pitch_cam));
    cameraFront = glm::normalize(front);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (fov >= 1.0f && fov <= 45.0f)
        fov -= yoffset;
    if (fov <= 1.0f)
        fov = 1.0f;
    if (fov >= 45.0f)
        fov = 45.0f;
}



int main(int argc, char *argv[]) {
    // GLFW Initialization
    if(!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return EXIT_FAILURE;
    }

    glfwSetErrorCallback(ErrorCallback);

    // hint GLFW that we would like an OpenGL 3 context (at least)
    // http://www.glfw.org/faq.html#how-do-i-create-an-opengl-30-context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // attempt to open the window: fails if required version unavailable
    // note some Intel GPUs do not support OpenGL 3.2
    // note update the driver of your graphic card
    GLFWwindow* window = glfwCreateWindow(window_width, window_height,
                                          "ICG project 1.0", NULL, NULL);
    if(!window) {
        glfwTerminate();
        return EXIT_FAILURE;
    }

    // makes the OpenGL context of window current on the calling thread
    glfwMakeContextCurrent(window);

    // set the callback for escape key
    glfwSetKeyCallback(window, key_callback);

    // set the framebuffer resize callback
    glfwSetFramebufferSizeCallback(window, SetupProjection);

    // set the mouse press and position callback
    glfwSetMouseButtonCallback(window, MouseButton);
    //glfwSetCursorPosCallback(window, MousePos);
    glfwSetScrollCallback(window, scroll_callback);

    // GLFW Options
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Set the required callback functions
    glfwSetCursorPosCallback(window, mouse_callback);

    // GLFW Options

    // GLEW Initialization (must have a context)
    // https://www.opengl.org/wiki/OpenGL_Loading_Library
    glewExperimental = GL_TRUE; // fixes glew error (see above link)
    if(glewInit() != GLEW_NO_ERROR) {
        fprintf( stderr, "Failed to initialize GLEW\n");
        return EXIT_FAILURE;
    }

    cout << "OpenGL" << glGetString(GL_VERSION) << endl;

    // initialize our OpenGL program
    Init(window);

    // update the window size with the framebuffer size (on hidpi screens the
    // framebuffer is bigger)
    glfwGetFramebufferSize(window, &window_width, &window_height);
    SetupProjection(window, window_width, window_height);

    // render loop
    while(!glfwWindowShouldClose(window)){
        // Calculate deltatime of current frame
        GLfloat currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Check and call events
        glfwPollEvents();
        do_movement();

        // Camera/View transformation
        glm::mat4 view;
        view_matrix = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        // Projection
        //glm::mat4 projection = glm::perspective(fov, (GLfloat)window_width/(GLfloat)window_height, 0.1f, 100.0f);
        // Get the uniform locations
        //GLint projLoc = glGetUniformLocation(grid., "projection");
        //glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        Display();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    quad.Cleanup();
    grid.Cleanup();
    skybox.Cleanup();
    water.Cleanup();


    // close OpenGL window and terminate GLFW
    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}


