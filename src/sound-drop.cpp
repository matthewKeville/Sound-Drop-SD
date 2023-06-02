#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>
#include <vector>
#include <thread>
#include <tuple>
#include "shader.cpp"
#include "Line.h"
#include "Ball.h"
#include "util.h"
#include "Spawner.h"
#include "soloud.h"
#include "soloud_wav.h"
#include "soloud_thread.h"

const int MAX_LINES = 50;
const int MAX_BALLS = 30; 

bool drawing = false;
double mouseX;                  //glfw window coordinates
double mouseY;

int windowWidth;                //glfw window dimensions
int windowHeight;
Shader* lineShader;
Shader* ballShader;

//lines
std::vector<Line*> lines;
float preview[2*3]{};           //vertex data for preview line


//balls
double ballSpawnX = -0.5f;
double ballSpawnY =  0.75f;
double ballRadius =  0.05f; 
double ballGravity =  -0.0002f;
double collisionRestitution = 0.95f;
double lastSpawn;         //time of last spawn
//double spawnRate = 1.0f;  //balls per second
//double spawnRate = 0.2f;  //balls per second
double spawnRate = 1.2f;  //balls per second
std::vector<Ball*> balls;

//Demo Spawner
Spawner* spawner;


SoLoud::Soloud soloud;
SoLoud::Wav sample;


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);

bool testLineSegmentIntersection(float xa0,float ya0,float xaf,float yaf,float xb0,float yb0,float xbf,float ybf,float* solx, float* soly);
void constrainedPreviewLineTerminal(float& xf,float& yf);

float toDegrees(float radians) {
  return radians * 360.0f / (2 * M_PI);
}

//transform mouseX to a view plane coordinate (-1.0,1.0)
float mouseXInView() {
  return (2.0f  * mouseX / (float) windowWidth  ) - 1.0f;
}

//transform mouseY to a view plane coordinate (-1.0,1.0)
float mouseYInView() {
  return (-2.0f * mouseY / (float) windowHeight ) + 1.0f;
}

void play_bounce_audio(Line* lp) {
  int handle = soloud.play(sample);
  int playback_rate = keville::util::semitone_adjusted_rate(keville::util::SAMPLE_BASE_RATE,lp->semitone);
  soloud.setSamplerate(handle,playback_rate);
}

void update_balls() {

  if ( balls.size() != MAX_BALLS ) {

    Ball* newBall = spawner->spawn(glfwGetTime());
    if ( newBall != nullptr ) {
      balls.push_back(newBall);
    }

  }

  auto itty = balls.begin();
  while (itty != balls.end()) {
    Ball* bp = *itty;

    float cxf = bp->cx; //end position
    float cyf = bp->cy;
    float vyf = bp->vy; //end velocity
    float vxf = bp->vx; 

    //free falling?
    vyf = bp->vy += ballGravity; 
    cxf = bp->cx + bp->vx;
    cyf = bp->cy + bp->vy;

    //collision with line?
    for ( auto lp : lines ) {
      float* lvert = lp->vertices;
      float solx;
      float soly;
      bool intersect = testLineSegmentIntersection(lvert[0],lvert[1],lvert[3],lvert[4],bp->cx,bp->cy,cxf,cyf,&solx,&soly);

      if (!intersect) {
        continue;
      }

      //threads must be detached or joined before the exit of the calling scope
      float line_width = sqrt(pow(lvert[3] - lvert[0],2) + pow(lvert[4] - lvert[1],2));
      std::thread audio_thread(play_bounce_audio,lp);
      audio_thread.detach();


      if ( lvert[3] == lvert[0] ) {
        //we just reflect the velocity vector on the x-axis
        vxf *= -1;
        cxf = bp->cx + vxf;
      } else {
        //find the normal vectors
        //we have two possible normals for our line segment, the right one
        //will have an obtuse angle the velocity vector, so we check the sign
        //of the dot product
        float line_slope = (lvert[4] - lvert[1]) / (lvert[3] - lvert[0]);
        float normal_slope = -(1/line_slope);
        //now we have a "normal" vector with <1,normal_slope> but we need to find the right orientation (sign)
        //compare sign of normal dot velocity
        bool flipped_normal = (1.0f * vxf) + (normal_slope * vyf) > 0; // ? 
        float normal_x = flipped_normal ? -1 : 1;
        float normal_y = normal_slope * (flipped_normal ? -1 : 1);
        float normal_angle = atan2(normal_y,normal_x); 

        float incline_angle = normal_angle - (M_PI/2);

        //find the incident angle between normal and the 'flipped' velocity vector
        float normal_dot_velocity = vxf * normal_x + vyf * normal_y;
        float normal_norm = sqrt(normal_x * normal_x + normal_y * normal_y);
        float velocity_norm = sqrt(vxf * vxf + vyf * vyf);
        float incident_angle = acos( normal_dot_velocity / ( normal_norm * velocity_norm ));
        if ( incident_angle > M_PI/2 ) { incident_angle = M_PI - incident_angle; }

        //do we add or subtract the incident angle to the normal angle?
        float velocity_angle = atan2(vyf,vxf);
        bool add_incidence = cos(velocity_angle - incline_angle) < 0;

        //find resultant angle
        float resultant_angle = atan2(normal_y,normal_x) + (add_incidence ? incident_angle  : -incident_angle );
        
        //rotate the velocity vector by this angle
        vxf = collisionRestitution * (velocity_norm * cos(resultant_angle));
        vyf = collisionRestitution * (velocity_norm * sin(resultant_angle) + ballGravity);
        cxf = bp->cx + vxf;
        cyf = bp->cy + vyf;

      }

    }

    //set the final state of the ball

    bp->vx = vxf;
    bp->vy = vyf;
    bp->cx = cxf;
    bp->cy = cyf;

    //should ball be deleted?
    if (bp->cy < -1.5) {
      delete bp;
      itty = balls.erase(itty);
    } else {
      itty++;
    }

  }

  //should a ball be deleted?
}

int main() {

  //initialize soloud
  soloud.init();

  //load sound samples
  sample.load("res/sine_440hz_44100_100ms.wav"); //44100
  keville::util::SAMPLE_BASE_RATE = 44100;
  //sample.load("res/glock_96000_1s.wav"); //44100
  //keville::util::SAMPLE_BASE_RATE = 96000;
  //sample.load("res/Highlight.wav"); // 32006;
  //SAMPLE_BASE_RATE = 32006;

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
  ballShader = new Shader("shaders/ball.vs","shaders/line.fs");

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

  //intersection buffer

  //general opengl settings

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); 

  spawner = new Spawner(ballShader,ballShader,spawnRate,ballSpawnX,ballSpawnY);

  
  while(!glfwWindowShouldClose(window))
  {
    processInput(window);
    glfwPollEvents();

    update_balls();
    //check_intersections();

    //clear..
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f); 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 


    //draw lines
    for (auto lp : lines) {
      lp->draw();
    }

    //draw preview
    if (drawing) {

      float xPos;
      float yPos;
      constrainedPreviewLineTerminal(xPos,yPos);

      preview[3] = xPos;
      preview[4] = yPos;
      preview[5] = 0.0f;

      float width = sqrt( pow(preview[4] - preview[1],2) + pow(preview[3] - preview[0],2));
      int semitone = keville::util::SEMITONE_WIDTH_MAP(width);
      std::tuple<float,float,float> color = keville::util::SEMITONE_COLOR_MAP(semitone);

      lineShader->use();
      int ColorLoc = glGetUniformLocation(lineShader->ID, "Color"); 
      glBindVertexArray(drawPreviewVao); 
      glBindBuffer(GL_ARRAY_BUFFER, drawPreviewVbo);
      glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(preview), preview);
      glUniform3f(ColorLoc,std::get<0>(color),std::get<1>(color),std::get<2>(color));
      glLineWidth(6.0f);
      glDrawArrays(GL_LINES, 0, 2);

    }

    //draw ball spawner
    spawner->draw();

    //draw balls
    for ( auto bp : balls ) {
      bp->draw();
    }
                                                                                               
    glfwSwapBuffers(window); 
  }

  //release glfw resources
  glfwTerminate();
  soloud.deinit();
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
  } 

}


void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
      if (lines.size() == MAX_LINES) {
        return;
      }
      if ( !drawing ) {

        //map mouse coordinates to view plane
        float xPos = mouseXInView();
        float yPos = mouseYInView();

        //write this vertex into the preview buffer
        preview[0] = xPos;
        preview[1] = yPos;
        preview[2] = 0;

        drawing = true;

      } else { 

        float xPos;
        float yPos;

        constrainedPreviewLineTerminal(xPos,yPos);
        
        //create a Line instance of this line

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


/* 
 * place the final coordinate of the preview line after being subjected
 * to the MAX and MIN constraints for lines 
 */
void constrainedPreviewLineTerminal(float& xPos,float& yPos) {

  xPos = mouseXInView();
  yPos = mouseYInView();

  //constrain the line between our bounds
  
  float vecX = xPos-preview[0];
  float vecY = yPos-preview[1];
  float distance = sqrt( pow(vecX,2) + pow(vecY,2) );


  if ( distance > keville::util::MAX_LINE_WIDTH ) {
    xPos = preview[0] + (vecX/distance)*keville::util::MAX_LINE_WIDTH;
    yPos = preview[1] + (vecY/distance)*keville::util::MAX_LINE_WIDTH;
  }

  if ( distance < keville::util::MIN_LINE_WIDTH ) {
    xPos = preview[0] + (vecX/distance)*keville::util::MIN_LINE_WIDTH;
    yPos = preview[1] + (vecY/distance)*keville::util::MIN_LINE_WIDTH;
  }

}

/*
 * this implementation has a flaw, when line segments are parallel to the x-axis, we get a nonzero slope, which is contradictory 
 * this is critical because when balls drop from the spawn the vector from there previous location to there future location
 * is precisely parallel to the y axis. As a bandage, ball spawn will be modified to have a tiny drift in the x direction in the absence
 * of a solution to this dillema
 */
bool testLineSegmentIntersection(float xa0,float ya0,float xaf,float yaf,float xb0,float yb0,float xbf,float ybf,float* solx, float* soly) {

  float dax = (xaf - xa0);
  float day = (yaf - ya0);
  if (!dax) { return false; } //divide by zero guard
  float ma  = day / dax;

  float dbx = (xbf - xb0);
  float dby = (ybf - yb0);
  if (!dbx) { return false; } //divide by zero guard
  float mb  = dby / dbx;
  
  //using the point slope equation for a line | y - y1 = m (x - x1) 
  //we find a solution to the intersection of our line segments extended to all of the Reals

  if (ma == mb) { return false; } //divide by zero guard
  float solvex = ( (ma * xa0) - (mb * xb0) + yb0 - ya0 );
  solvex/=(ma-mb);

  //now we check if solx is in the intersection of our segments x domains
  float axmin = std::min(xa0,xaf);
  float axmax = std::max(xa0,xaf);
  float bxmin = std::min(xb0,xbf);
  float bxmax = std::max(xb0,xbf);

  if ( solvex > axmin && solvex < axmax 
        && solvex > bxmin && solvex < bxmax ) {
    *solx = solvex;
    *soly = ma * (solvex - xa0) + ya0;
    return true;
  }

  solx = 0;
  soly = 0;
  return false;


}

