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
#include "Interactable.h"
#include "soloud.h"
#include "soloud_wav.h"
#include "soloud_thread.h"

const int MAX_LINES = 50;
const int MAX_BALLS = 30; 

int windowWidth;                //glfw window dimensions
int windowHeight;
GLFWwindow* window;

//state

bool lineDrawing = false;
bool selected = false;
bool pausePhysics = false;
bool muteAudio = false;

double mouseX;                  //glfw window coordinates
double mouseY;

Shader* lineShader;
Shader* ballShader;

//lines
std::vector<Line*> lines;
//preview vars
float preview[2*3]{};           
unsigned int drawPreviewVao;
unsigned int drawPreviewVbo;

//balls
double ballGravity =  -0.0002f;
double collisionRestitution = 0.95f;
std::vector<Ball*> balls;

double DEFAULT_SPAWN_X = -0.5f;
double DEFAULT_SPAWN_Y = 0.5f;
//double DEFAULT_SPAWN_F = 1.0f;
float DEFAULT_BASE_SPAWN_FREQUENCY = (1.0f/6.0f);//0.25f;
float DEFAULT_SPAWN_SCALE = 1.0f;
float SPAWNER_SCALE_MAX = 5.0f;
float SPAWNER_SCALE_MIN = 1.0f;
std::vector<Spawner*> spawners;

//Interactable (the selected entity)
Interactable* hovered      = nullptr;
Interactable* interactable = nullptr; 


//Audio
SoLoud::Soloud soloud;
SoLoud::Wav sample;

//Keys
bool S_PRESSED = false;
bool P_PRESSED = false;
bool M_PRESSED = false;
bool UP_PRESSED = false;
bool DOWN_PRESSED = false;


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);

void drawLinePreview();
bool testLineSegmentIntersection(float xa0,float ya0,float xaf,float yaf,float xb0,float yb0,float xbf,float ybf,float* solx, float* soly);
void constrainedPreviewLineTerminal(float& xf,float& yf);


float toDegrees(float radians) {
  return radians * 360.0f / (2 * M_PI);
}

//transform mouseX to a view plane coordinate (-1.0,1.0)
double mouseXToViewX(double mousex) {
  return (2.0f  * mousex / (double) windowWidth  ) - 1.0f;
}

//transform mouseY to a view plane coordinate (-1.0,1.0)
double mouseYToViewY(double mousey) {
  return (-2.0f * mousey / (double) windowHeight ) + 1.0;
}

void play_bounce_audio(Line* lp) {
  int handle = soloud.play(sample);
  int playback_rate = keville::util::semitone_adjusted_rate(keville::util::SAMPLE_BASE_RATE,lp->semitone);
  soloud.setSamplerate(handle,playback_rate);
}

//return the hovered interactable if any
Interactable* detect_hover() {

  for ( auto sp : spawners ) {
    if ( sp->IsHovering(mouseXToViewX(mouseX),mouseYToViewY(mouseY)) ) {
      return sp;
    }
  }

  for ( auto lp : lines ) {
    if ( lp->IsHovering(mouseXToViewX(mouseX),mouseYToViewY(mouseY)) ) {
      return lp;
    }
  }
  return nullptr;
}

//the interactable gets moved by the user when selected
void update_interactable() {
  if ( !selected ) return;
  // this should be interactable->move(dx,dy);
  // but I was having difficulty implementing smooth movement
  // when considered the dynamics between rendering a the glfw calculation of position and 
  // the callbacks. This is the corect, not jank way. We set the position as a workaround.
  interactable->position(mouseXToViewX(mouseX),mouseYToViewY(mouseY));

}

void update_balls() {

  //ball spawners
  auto sitty = spawners.begin();
  while (sitty != spawners.end() && balls.size() != MAX_BALLS ) {
    Spawner* sp = *sitty;
    Ball* newBall = sp->spawn(glfwGetTime());
    if ( newBall != nullptr ) {
      balls.push_back(newBall);
    }
    sitty++;
  }

  //balls
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

      if (!muteAudio) {
        //threads must be detached or joined before the exit of the calling scope
        float line_width = sqrt(pow(lvert[3] - lvert[0],2) + pow(lvert[4] - lvert[1],2));
        std::thread audio_thread(play_bounce_audio,lp);
        audio_thread.detach();
      }


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
  //keville::util::SAMPLE_BASE_RATE = 32006;

  //initialize glfw

  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  window = glfwCreateWindow(800, 600, "Drop Sound", NULL, NULL);

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
  glGenVertexArrays(1, &drawPreviewVao);
  glGenBuffers(1,&drawPreviewVbo);
  glBindBuffer(GL_ARRAY_BUFFER, drawPreviewVbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * 3, preview, GL_STATIC_DRAW);//stores 2 lines
  glBindVertexArray(drawPreviewVao);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);//positions
  glEnableVertexAttribArray(0); 
  glBindVertexArray(0);

  //default spawner
  /*spawners.push_back(new Spawner(ballShader,ballShader,
        DEFAULT_SPAWN_X,DEFAULT_SPAWN_Y,
        DEFAULT_BASE_SPAWN_FREQUENCY,DEFAULT_SPAWN_SCALE));
  */
  spawners.push_back(new Spawner(ballShader,ballShader,
        DEFAULT_SPAWN_X,DEFAULT_SPAWN_Y,
        DEFAULT_BASE_SPAWN_FREQUENCY,4.0f));
  
  while(!glfwWindowShouldClose(window))
  {

    ///////////////////////////////
    //INPUT
    ///////////////////////////////
    
    hovered = detect_hover();
    processInput(window);
    glfwPollEvents();


    ///////////////////////////////
    //DELETE
    ///////////////////////////////

    //what needs to be deleted?
      //an event system would be better, here we could read through the delete event buffer
      //as opposed to this poorly scaling method.


      auto itty = lines.begin();
      while (itty != lines.end()) {
        Interactable* lp = *itty;
        if ( lp->isDeleted() ) {
          delete lp;
          itty = lines.erase(itty);
        } else {
          itty++;
        }
      }

      auto sitty = spawners.begin();
      while (sitty != spawners.end()) {
        Interactable* sp = *sitty;
        if ( sp->isDeleted() ) {
          delete sp;
          sitty = spawners.erase(sitty);
        } else {
          sitty++;
        }
      }


    ///////////////////////////////
    //UPDATE
    ///////////////////////////////

    if (!pausePhysics) {
      update_balls();
    }
    update_interactable();

    ///////////////////////////////
    //RENDER
    ///////////////////////////////

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f); 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

    //draw lines
    for (auto lp : lines) {
      lp->draw();
    }

    //draw preview
    if (lineDrawing) {
      drawLinePreview();
    }

    //draw ball spawner
    for ( auto sp : spawners ) {
      sp->draw();
    }

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

void drawLinePreview(){
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

void processInput(GLFWwindow* window) {

  ///////////////////////////////
  //PLAY/PAUSE
  ///////////////////////////////
  if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
    if (!P_PRESSED) {
      pausePhysics = !pausePhysics;
      P_PRESSED = true;
      std::cout << "Simulation " << (pausePhysics ? " Paused " : " Resume ")  << std::endl;
    }
  }
  if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE) {
    P_PRESSED = false;
  }

  ///////////////////////////////
  //ESCAPE (ESC)
  ///////////////////////////////
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    selected = false;
    interactable = nullptr;
    std::cout << "Interaction aborted" << std::endl;
  }

  ///////////////////////////////
  //DELETE (D)
  ///////////////////////////////
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    if (selected) {
      std::cout << "Deleting Interactable" << std::endl;
      //what kind of interactable is it (where is it stored)
      //this is bad and needs to be refactored
      if ( dynamic_cast<Line*>(interactable) != nullptr ) {
        interactable->markDeleted();
      }
      if ( dynamic_cast<Spawner*>(interactable) != nullptr ) {
        interactable->markDeleted();
      }

    }
    selected = false;
    interactable = nullptr;
  }

  ///////////////////////////////
  //Change the scale of spawner (UP)
  ///////////////////////////////
  if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
    if (!UP_PRESSED) {
      //if this is a spawner adjust the scale
      if (selected) {
        Spawner* sp = dynamic_cast<Spawner*>(interactable);
        if ( sp != nullptr ) {
          unsigned int current = sp->getScale();
          if (current != SPAWNER_SCALE_MAX) {
            sp->setScale(current+1);
          }     
          UP_PRESSED = true;
          std::cout << "increasing spawner" << std::endl;
        }
      }
    }
  }
  if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_RELEASE) {
    UP_PRESSED = false;
  }

  ///////////////////////////////
  //Change the scale of spawner (DOWN)
  ///////////////////////////////
  if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
    if (!DOWN_PRESSED) {
      if (selected) {
        Spawner* sp = dynamic_cast<Spawner*>(interactable);
        if ( sp != nullptr ) {
          unsigned int current = sp->getScale();
          if (current != SPAWNER_SCALE_MIN) {
            sp->setScale(current-1);
          }     
          DOWN_PRESSED = true;
          std::cout << "decreasing spawner" << std::endl;
        }
      }
    }
  }
  if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_RELEASE) {
    DOWN_PRESSED = false;
  }



  ///////////////////////////////
  //SPAWN (S)
  ///////////////////////////////
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
    if (!S_PRESSED) {
      if (!selected && hovered == nullptr) {
        spawners.push_back(new Spawner(ballShader,ballShader,mouseXToViewX(mouseX),mouseYToViewY(mouseY),
          DEFAULT_BASE_SPAWN_FREQUENCY,DEFAULT_SPAWN_SCALE));
      }
      S_PRESSED = true;
      std::cout << "spawning spawner" << std::endl;
    }
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_RELEASE) {
    S_PRESSED = false;
  }


  ///////////////////////////////
  //MUTE (M)
  ///////////////////////////////
  if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
    if (!M_PRESSED) {
      muteAudio = !muteAudio;
      M_PRESSED = true;
      soloud.stopAll();
      std::cout << (muteAudio ? " Muting " : " UnMuting ")  << std::endl;
    }
  }
  if (glfwGetKey(window, GLFW_KEY_M) == GLFW_RELEASE) {
    M_PRESSED = false;
  }


  //////////////////////////////
  //Clear (C)
  //////////////////////////////
  if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {

    auto sitty = spawners.begin();
    while (sitty != spawners.end()) {
      Spawner* sp = *sitty;
      sitty = spawners.erase(sitty);
      delete sp;
    }

    auto bitty = balls.begin();
    while (bitty != balls.end()) {
      Ball* bp = *bitty;
      bitty = balls.erase(bitty);
      delete bp;
    }

    auto litty = lines.begin();
    while (litty != lines.end()) {
      Line* lp = *litty;
      litty = lines.erase(litty);
      delete lp;
    }

    std::cout << "Clearing Entities" << std::endl;


  } 

  ///////////////////////////////
  //QUIT (q)
  ///////////////////////////////
  if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window,true);
  } 


}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {

      ////////////////////////////////
      //Line Drawing Logic
      ////////////////////////////////
      if ( !selected && hovered==nullptr ) {
        
        if (lines.size() == MAX_LINES) {
          return;
        }

        if ( !lineDrawing ) {

          //map mouse coordinates to view plane
          float xPos = mouseXToViewX(mouseX);
          float yPos = mouseYToViewY(mouseY);

          //write this vertex into the preview buffer
          preview[0] = xPos;
          preview[1] = yPos;
          preview[2] = 0;

          lineDrawing = true;

        } else { 

          float xPos;
          float yPos;

          constrainedPreviewLineTerminal(xPos,yPos);
          
          //create a Line instance of this line

          lines.push_back(new Line(lineShader,preview[0],preview[1],xPos,yPos));
          lineDrawing = false;

        }

      ////////////////////////////////
      //interactable selection 
      ////////////////////////////////
      } else if ( !selected && hovered!=nullptr ) {
        selected = true;
        interactable = hovered;
        std::cout << "begging interaction" << std::endl;

      } else if ( selected ) {
        selected = false;
        interactable = nullptr;
        std::cout << "ending interaction" << std::endl;
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

  xPos = mouseXToViewX(mouseX);
  yPos = mouseYToViewY(mouseY);

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
 * this is critical because when a balls drop from the spawn the vector from there previous location to there future location
 * is precisely parallel to the y axis. As a bandage, ball spawn will be modified to have a tiny drift in the x direction in the absence
 * of a solution to this dillema. TODO
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

