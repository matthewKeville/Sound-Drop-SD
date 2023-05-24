#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>
#include <vector>
#include "shader.cpp"

const int MAX_LINES = 10;
int lines = 2;                  //current line count

float vertices[MAX_LINES*2*3];  //vertex data for current lines
float preview[2*3];             //vertex data for preview line

bool drawing = false;
double mouseX;                  //glfw window coordinates
double mouseY;

int windowWidth;                //glfw window dimensions
int windowHeight;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);

void dump_state() {
}

//transform mouseX to a view plane coordinate (-1.0,1.0)
float mouseXInView() {
  return (2.0f  * mouseX / (float) windowWidth  ) - 1.0f;
}

//transform mouseY to a view plane coordinate (-1.0,1.0)
float mouseYInView() {
  return (-2.0f * mouseY / (float) windowHeight ) + 1.0f;
}

int main() {

  //initialize glfw

  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  GLFWwindow* window = glfwCreateWindow(800, 600, "Drop Sound", NULL, NULL);

  if ( window == NULL ) 
  {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  //initialize GLAD

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) 
  {
    std::cout << "Failed to init GLAD" << std::endl;
    return -2;
  }

  glViewport(0, 0, 800, 600);
  glfwGetWindowSize(window,&windowWidth,&windowHeight);

  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetCursorPosCallback(window, mouse_callback);

  //load shader programs

  Shader shader0("shaders/line.vs","shaders/line.fs");

  //load default lines data
  
  vertices[0] = -0.5f;
  vertices[1] = -0.5f;
  vertices[2] =  0.0f;
  vertices[3] =  0.5f;
  vertices[4] =  0.5f;
  vertices[5] =  0.0f;

  vertices[6] =  0.5f;
  vertices[7] = -0.5f;
  vertices[8] =  0.0f;
  vertices[9] = -0.5f;
  vertices[10] = 0.5f;
  vertices[11] =  0.0f;

  preview[0] = 0.0f;
  preview[1] = 0.0f;
  preview[2] = 0.0f;
  preview[3] = 0.0f;
  preview[4] = 0.0f;
  preview[5] = 0.0f;

  //established lines

  unsigned int vao;
  unsigned int vbo;
  glGenVertexArrays(1, &vao);
  glGenBuffers(1,&vbo);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * MAX_LINES * 2 * 3, vertices, GL_STATIC_DRAW); //stores "MAX_LINES" lines

  glBindVertexArray(vao);
  //glBindBuffer(GL_ARRAY_BUFFER, vbo); //this isn't necessary?
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0); 
  glBindVertexArray(0);

  //preview line

  unsigned int drawPreviewVao;
  unsigned int drawPreviewVbo;
  glGenVertexArrays(1, &drawPreviewVao);
  glGenBuffers(1,&drawPreviewVbo);

  glBindBuffer(GL_ARRAY_BUFFER, drawPreviewVbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * 3, preview, GL_STATIC_DRAW);//stores 2 lines

  glBindVertexArray(drawPreviewVao);
  //glBindBuffer(GL_ARRAY_BUFFER, drawPreviewVbo); //this isn't necessary?
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);//positions
  glEnableVertexAttribArray(0); 

  //general opengl settings

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); 
  glLineWidth(3.0f);
  
  while(!glfwWindowShouldClose(window))
  {
    processInput(window);

    //clear..
    glClearColor(0.67f, 0.18f, 0.93f, 1.0f); 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

    //render lines
    shader0.use();
    glBindVertexArray(vao); 
    //modify lines buffer (should be a check here to see if data has changed...)
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 2 * 3 * lines, vertices);
    //lines color
    int ColorLoc = glGetUniformLocation(shader0.ID, "Color"); 
    glUniform3f(ColorLoc,0.0f,1.0f,1.0f);

    glDrawArrays(GL_LINES, 0, lines*2);

    //also render a drawing preview
    if (drawing) {
      //convert mouse coordinates to view plane coordinates
      float xPos = mouseXInView();
      float yPos = mouseYInView();
      preview[3] = xPos;
      preview[4] = yPos;
      preview[5] = 0.0f;

      glBindVertexArray(drawPreviewVao); 
      glBindBuffer(GL_ARRAY_BUFFER, drawPreviewVao);
      glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(preview), preview);
      glUniform3f(ColorLoc,1.0f,0.0f,0.0f);
      glDrawArrays(GL_LINES, 0, 2);
    }
                                                                                               
    glfwPollEvents();
    glfwSwapBuffers(window); 
  }

  //release glfw resources
  glfwTerminate();
  return 0;

}

//we want to resize the viewport to scale with the glfw window
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
  glViewport(0,0, width, height);
  glfwGetWindowSize(window,&windowWidth,&windowHeight);
  //std::cout << "window width " << windowWidth << " window height " << windowHeight << std::endl;
}

void processInput(GLFWwindow* window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window,true);
  if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
    dump_state();
  }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
      if (lines == MAX_LINES) {
        return;
      }
      if ( !drawing ) {
        //anchor coordinate set
        std::cout << "line drawing start" << std::endl;

        //map mouse coordinates to view plane
        float xPos = mouseXInView();
        float yPos = mouseYInView();

        //write this vertex into the preview buffer
        preview[0] = xPos;
        preview[1] = yPos;
        preview[2] = 0;

        drawing = true;

        /*
        std::cout << "new line at" << std::endl;
        char msg[120];
        sprintf(msg,"%f, %f",xPos,yPos);
        std::cout << msg << std::endl;
        */

      } else { 
        //the second coordinate has been selected
        std::cout << "line drawing end" << std::endl;

        //move the preview line into lines
        int index = lines*2*3;
        vertices[index] = preview[0];
        vertices[index+1] = preview[1];
        vertices[index+2] = preview[2];

        float xPos = mouseXInView();
        float yPos = mouseYInView();
        vertices[index+3] = xPos;
        vertices[index+4] = yPos;
        vertices[index+5] = 0.0f;

        lines++;
        drawing = false;

        /*
        std::cout << "new line at" << std::endl;
        char msg[120];
        sprintf(msg,"%f, %f",xPos,yPos);
        std::cout << msg << std::endl;
        */

      }
    }

    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
      if ( lines != 0 ) {
        std::cout << "Removing a Line" << std::endl;
        lines--; 
      } else {
        std::cout << "No more lines to remove" << std::endl;
      }
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
  mouseX = xpos;
  mouseY = ypos;
}

