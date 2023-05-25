#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>
#include <vector>
#include "shader.cpp"
#include "Line.h"

const int MAX_LINES = 12;
//lines
std::vector<Line*> lines;
float preview[2*3];             //vertex data for preview line

bool drawing = false;
double mouseX;                  //glfw window coordinates
double mouseY;

int windowWidth;                //glfw window dimensions
int windowHeight;
Shader* lineShader;
Shader* lineShader2;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);


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

  lineShader = new Shader("shaders/line.vs","shaders/line.fs");

  //load default preview data
  
  preview[0] = 0.0f;
  preview[1] = 0.0f;
  preview[2] = 0.0f;
  preview[3] = 0.0f;
  preview[4] = 0.0f;
  preview[5] = 0.0f;

  //preview line

  unsigned int drawPreviewVao;
  unsigned int drawPreviewVbo;
  glGenVertexArrays(1, &drawPreviewVao);
  glGenBuffers(1,&drawPreviewVbo);

  glBindBuffer(GL_ARRAY_BUFFER, drawPreviewVbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * 3, preview, GL_STATIC_DRAW);//stores 2 lines

  glBindVertexArray(drawPreviewVao);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);//positions
  glEnableVertexAttribArray(0); 
  glBindVertexArray(0);

  //general opengl settings

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); 
  glLineWidth(3.0f);
  
  while(!glfwWindowShouldClose(window))
  {
    processInput(window);
    glfwPollEvents();

    //clear..
    glClearColor(0.07f, 0.0f, 0.93f, 0.0f); 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

    //draw lines
    for (auto lp : lines) {
      lp->draw();
    }

    //draw preview
    if (drawing) {

      //convert mouse coordinates to view plane coordinates
      float xPos = mouseXInView();
      float yPos = mouseYInView();
      preview[3] = xPos;
      preview[4] = yPos;
      preview[5] = 0.0f;

      lineShader->use();
      int ColorLoc = glGetUniformLocation(lineShader->ID, "Color"); 
      glBindVertexArray(drawPreviewVao); 
      glBindBuffer(GL_ARRAY_BUFFER, drawPreviewVbo);
      glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(preview), preview);
      glUniform3f(ColorLoc,1.0f,0.0f,0.0f);
      glDrawArrays(GL_LINES, 0, 2);

    }


                                                                                               
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
}

void processInput(GLFWwindow* window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window,true);
  if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
    for (auto lp: lines) {
      lp->print();
    }
  }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
      if (lines.size() == MAX_LINES) {
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

      } else { 
        //the second coordinate has been selected

        float xPos = mouseXInView();
        float yPos = mouseYInView();

        std::cout << "line drawing end" << std::endl;
        char msg[120];
        sprintf(msg,"x0 %f y0 %f xf %f yf %f",preview[0],preview[1],xPos,yPos);
        std::cout << msg << std::endl;

        lines.push_back(new Line(lineShader,preview[0],preview[1],xPos,yPos));

        drawing = false;

      }
    }

    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
      if ( lines.size() != 0 ) {
        std::cout << "Removing a Line" << std::endl;
        Line* lastLine = lines[lines.size()-1];
        delete lastLine;
        lines.pop_back();
      } else {
        std::cout << "No more lines to remove" << std::endl;
      }
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
  mouseX = xpos;
  mouseY = ypos;
}

