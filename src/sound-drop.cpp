#include <iostream>
#include <cmath>
#include <vector>
#include <thread>
#include <tuple> 
#include <functional>
#include <algorithm> //std::copy
#include <chrono>    //sleep thread
#include <filesystem>
#include <fstream>
#include <regex>
//Third Party
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp> //to use <<() : extraction operator on glm data types
#include "soloud.h"
#include "soloud_wav.h"
#include "soloud_thread.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "stb_image.h" //image loading
//keville
#include "shader.cpp"
#include "Line.h"
#include "Ball.h"
#include "util.h"
#include "Spawner.h"
#include "Interactable.h"
#include "SaveState.h"
#include "StateStack.h"

#if defined DEV_BUILD
  const std::string RES_PATH = "./res";
  const std::string DATA_PATH = "./.sound-drop";
#else
  const std::string RES_PATH = "/usr/local/share/sound-drop-sd/res";
  const std::string DATA_PATH = "~/.sound-drop";
#endif
std::filesystem::path selectedLoadFile = "";

const int MAX_LINES = 50;
const int MAX_BALLS = 300; 

//GLFW
int windowWidth;                
int windowHeight;
GLFWwindow* window;
glm::vec2 mouse; //glfw window coordinates : (0,windowWidth) x (0,windowHeight)
const int vsync = 0;
const int MSAA = 16; //should this be lifted to a user setting?

//OPENGL
const auto frameTarget = std::chrono::milliseconds(16);// ~60 fps (application logic)
const float MAX_VIEWPORT_SCALE = 3.0f;
const float MIN_VIEWPORT_SCALE = 1.0f;
float viewportScale = MIN_VIEWPORT_SCALE;
glm::vec2 viewportCenter = glm::vec2(0,0);
glm::vec4 clearColor{0.0f,0.0f,0.0f,0.0f};
glm::mat4 projection = glm::ortho(-viewportScale, viewportScale, -viewportScale, viewportScale, -10.f, 10.0f); //l,r,b,t,n,f
glm::mat4 view = glm::mat4(1); 

//shaders
Shader* lineShader;
Shader* ballShader;
Shader* digitShader;

//lines
std::vector<Line*> lines;
const float MIN_LINE_THICKNESS=0.001f;
const float MAX_LINE_THICKNESS=0.01f;
float lineThickness=0.005f;
glm::mat4 thicken = glm::scale(glm::mat4(1),glm::vec3(1,lineThickness,1));
unsigned int lineVao;
unsigned int lineVbo;
float lineVertices[18] {0}; 

Line* previewLine;
glm::vec2 previewBasePoint = viewportCenter;

//balls
unsigned int ballVao;
unsigned int ballVbo;
float* ballVertices;
                     
const double GRAVITY_BASE =  -0.0002f;
double ballGravityScale = 0;
double ballGravity = GRAVITY_BASE;
double collisionRestitution = 0.95f;
std::vector<Ball*> balls;
                                                   
//spawners
unsigned int spawnerVao;
unsigned int spawnerVbo;
float* spawnerVertices;

double DEFAULT_SPAWN_X = -0.5f;
double DEFAULT_SPAWN_Y = 0.5f;
float DEFAULT_BASE_SPAWN_FREQUENCY = (1.0f/3.0f);
unsigned int DEFAULT_SPAWN_SCALE = 1;
float SPAWNER_SCALE_MAX = 8.0f;
float SPAWNER_SCALE_MIN = 1.0f;
std::vector<Spawner*> spawners;

//digits
unsigned int digitVao;
unsigned int digitVbo;
float digitVertices[30] {0};
unsigned int digitTextures[10] = {0};

//Interactable (the selected entity)
Interactable* hovered      = nullptr;
Interactable* interactable = nullptr; 
glm::vec2 interactableCenterDisplacement = glm::vec2(0,0); /* world space vector from center to unprojected click */

//Audio
SoLoud::Soloud soloud;
SoLoud::Wav* sample;
unsigned int sampleIndex = 0;
std::vector<std::tuple<std::/*Name*/string,/*Rate*/unsigned int>> sampleData;
unsigned int SAMPLE_BASE_RATE = 0;
const float GLOBAL_MAX_VOLUME = 2;
const float GLOBAL_MIN_VOLUME = 0;
const float GLOBAL_DEFAULT_VOLUME = 1.0f;
const unsigned int MAX_VOICE_COUNT = 255;//1024 is standard max, but library supports 4095 with compilation flag
                                         //yet, compiler will yell if this is above 255 ?
//scales
unsigned int scaleIndex = 0;
std::vector<
  std::tuple<
    std::string,
    std::function<int(float width)>, std::function<std::tuple<float,float,float>(int semitones)>
  >
> scaleData;

//saving
const unsigned int NUM_SAVE_SLOTS = 3;
int selectedSaveSlot = 0;
SaveState saveSlots[NUM_SAVE_SLOTS];
StateStack stateStack;

//sim states
bool lineDrawing = false;
bool selected = false;
bool pausePhysics = false;
bool showHelp = true;
bool showConfig = true;
bool muteAudio = false;
                                                              
//key states (for combo clicks like zoom in/out)
bool ALT_PRESSED = false;
bool L_CONTROL_PRESSED = false;

//glfw callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);

//application routines
void updateSample();
void updateScale();
void constrainedPreviewLineTerminal(float& xf,float& yf);
void drawLinePreview();
bool testLineSegmentIntersection(float xa0,float ya0,float xaf,float yaf,float xb0,float yb0,float xbf,float ybf,float* solx, float* soly);
void play_bounce_audio(Line* lp);
Interactable* detect_hover();
void update_interactable();
void update_balls();
void updateView(); 
void changeView(glm::vec2); 
void updateProjection(); 
void changeProjection(float); 
void resetViewport();
void saveToFile(std::vector<Line*>& lines, std::vector<Spawner*>&,std::string);
void loadFromFile(std::vector<Line*>& lines, std::vector<Spawner*>&,std::filesystem::path);

void init();
void generateSampleData();
void generateScaleData();
bool validateWavFile(std::filesystem::path);
void shutdown();

//render loop abstractions
void update();
void processIO();
void processGUI();
void processConfigGui(ImGuiIO& io);
void processHelpGui(ImGuiIO& io);
void draw();

//state stack manipulation
void record();
void undo();
void redo();

//utilities
float toDegrees(float radians);
glm::vec2 mouseToNDC(glm::vec2 mouse);
glm::vec2 ndcToWorldCoordinates(glm::vec2 ndc); 
float viewportCenterRange();

int main() {

  init();
  record();

  while(!glfwWindowShouldClose(window))
  {

    std::chrono::time_point<std::chrono::system_clock> frameStart = 
        std::chrono::system_clock::now();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
    processIO();
    update();
    draw();
    processGUI();

    std::chrono::time_point<std::chrono::system_clock> frameEnd = 
        std::chrono::system_clock::now();

    auto frameTime = frameEnd - frameStart;
    auto frameTime_ms = std::chrono::duration_cast<std::chrono::milliseconds>(frameTime);

    auto clearance = frameTarget - frameTime;
    auto clearance_ms = std::chrono::duration_cast<std::chrono::milliseconds>(clearance);

    //std::cout << " frame time ms : " << frameTime_ms.count() << std::endl;
    //std::cout << " clearance  ms: " << clearance_ms.count() << std::endl;

    if ( clearance.count() >= 0  ) {
      std::this_thread::sleep_for(clearance);
    } else {
      //std::cerr << "Warning : Can't keep up" << clearance_ms.count() << std::endl;
    }

    glfwSwapBuffers(window); 

  }

  shutdown();
  return 0;

}

//we want to resize the viewport to scale with the glfw window
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
  glViewport(0,0, width, height);
  glfwGetWindowSize(window,&windowWidth,&windowHeight);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) 
{
  ImGuiIO& io = ImGui::GetIO();
  if (io.WantCaptureKeyboard || io.WantCaptureMouse) {
    return;
  }

  //////////////////
  // mod keys
  //////////////////

  if (key == GLFW_KEY_LEFT_ALT ) {
    if (action == GLFW_PRESS ) {
      std::cout << " mod key alt enabled " << std::endl;
      ALT_PRESSED = true;
    } else if (action == GLFW_RELEASE) {
      std::cout << " mod key alt disabled " << std::endl;
      ALT_PRESSED = false;
    }
  }

  if (key == GLFW_KEY_LEFT_CONTROL ) {
    if (action == GLFW_PRESS ) {
      std::cout << " mod key control enabled " << std::endl;
      L_CONTROL_PRESSED = true;
    } else if (action == GLFW_RELEASE) {
      std::cout << " mod key control disabled " << std::endl;
      L_CONTROL_PRESSED = false;
    }
  }

  //////////////////
  // normal mode
  //////////////////

  if (!selected) {

    //show/hide help
    if (key == GLFW_KEY_H && action == GLFW_RELEASE) {
        showHelp = !showHelp;
        std::cout << "Help " << (showHelp ? " show " : " hide ")  << std::endl;
    }

    //show/hide config menu
    if (key == GLFW_KEY_C && action == GLFW_RELEASE) {
        showConfig = !showConfig;
        std::cout << "Config " << (showConfig ? " show " : " hide ")  << std::endl;
    }

    //pause
    if (key == GLFW_KEY_P && action == GLFW_RELEASE) {
        pausePhysics = !pausePhysics;
        std::cout << "Simulation " << (pausePhysics ? " Paused " : " Resume ")  << std::endl;
    }

    //Pan Left
    if (key == GLFW_KEY_LEFT && action == GLFW_RELEASE) {
        std::cout << "panning left" << std::endl;
        changeView(viewportCenter + glm::vec2(0.1f,0.0f));
        updateView();
    } 

    //Pan Right
    if (key == GLFW_KEY_RIGHT  && action == GLFW_RELEASE) {
        std::cout << "panning right" << std::endl;
        changeView(viewportCenter + glm::vec2(-0.1f,0.0f));
        updateView();
    } 

    //Zoom out / Pan up
    if (key == GLFW_KEY_UP  && action == GLFW_RELEASE) {
      if (ALT_PRESSED) {
        std::cout << "zooming out" << std::endl;
        changeProjection(viewportScale*(1.1f));
        updateProjection();
      } else {
        std::cout << "panning up" << std::endl;
        changeView(viewportCenter + glm::vec2(0,-0.1f));
        updateView();
      }
    }

    //Zoom in / Pan Down
    if (key == GLFW_KEY_DOWN  && action == GLFW_RELEASE) {
      if (ALT_PRESSED) {
        std::cout << "zooming in" << std::endl;
        changeProjection(viewportScale*(0.9f));
        updateProjection();
      } else {
        std::cout << "panning down" << std::endl;
        changeView(viewportCenter + glm::vec2(0,0.1f));
        updateView();
      }
    }



    //redo
    if (key == GLFW_KEY_U && action == GLFW_RELEASE) {
        undo();
    } 

    //undo
    if (key == GLFW_KEY_R && action == GLFW_RELEASE) {
        if ( L_CONTROL_PRESSED ) { 
          redo();
        } else {
          resetViewport();
        }
    } 

    //new spawner
    if (key == GLFW_KEY_S && action == GLFW_RELEASE) {
        if (!selected && hovered == nullptr) {  // Spawn
          auto wscp = ndcToWorldCoordinates(mouseToNDC(mouse));
          spawners.push_back(new Spawner( /*spawner shader is ball shader*/ballShader, &spawnerVao, &spawnerVbo,
                                          digitShader, &digitVao, &digitVbo, digitTextures,
                                          ballShader, &ballVao,&ballVbo,
                                          wscp.x,wscp.y,
                                          DEFAULT_BASE_SPAWN_FREQUENCY,DEFAULT_SPAWN_SCALE));
          record();
        }
        std::cout << "spawning spawner" << std::endl;
    }

    //mute
    if (key == GLFW_KEY_M && action == GLFW_RELEASE) {
        muteAudio = !muteAudio;
        soloud.stopAll();
        std::cout << (muteAudio ? " Muting " : " UnMuting ")  << std::endl;
    }

    //exit
    if (key == GLFW_KEY_Q && action == GLFW_RELEASE) {
      glfwSetWindowShouldClose(window,true);
    } 

  ///////////////////////////
  // interaction mode
  ///////////////////////////

  } else if (selected) {

    //increase spawn rate
    if (key == GLFW_KEY_UP && action == GLFW_RELEASE) {
        Spawner* sp = dynamic_cast<Spawner*>(interactable);
        if ( sp != nullptr ) {
          unsigned int current = sp->getScale();
          if (current != SPAWNER_SCALE_MAX) {
            sp->setScale(current+1);
          }     
          std::cout << "increasing spawner" << std::endl;
        }
    }

    //decrease spawn rate
    if (key == GLFW_KEY_DOWN && action == GLFW_RELEASE) {
        Spawner* sp = dynamic_cast<Spawner*>(interactable);
        if ( sp != nullptr ) {
          unsigned int current = sp->getScale();
          if (current != SPAWNER_SCALE_MIN) {
            sp->setScale(current-1);
          }     
          std::cout << "decreasing spawner" << std::endl;
        }
    }

    //delete
    if (key == GLFW_KEY_D && action == GLFW_RELEASE) {
      std::cout << "Deleting Interactable" << std::endl;
      if ( dynamic_cast<Line*>(interactable) != nullptr ) {
        interactable->markDeleted();
      }
      if ( dynamic_cast<Spawner*>(interactable) != nullptr ) {
        interactable->markDeleted();
      }
      selected = false;
      interactable = nullptr;
    }

  }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    (void) window;//suppress -Wunused-paramter
    (void) mods;//suppress -Wunused-paramter
    //if ImGui window "Wants" the mouse, we do not process below
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
      return;
    }
  
    //draw lines / pick selection
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {

      //draw lines
      if ( !selected && hovered==nullptr ) {
        if (lines.size() == MAX_LINES) {
          return;
        }
        if ( !lineDrawing ) {
          //map mouse coordinates to view plane
          previewBasePoint = ndcToWorldCoordinates(mouseToNDC(mouse));
          lineDrawing = true;
        } else { 
          float xPos; float yPos;
          constrainedPreviewLineTerminal(xPos,yPos);
          //create a Line instance of this line
          auto [name , semitoneMapper, colorMapper ] = scaleData[scaleIndex];
          lines.push_back(new Line(lineShader,&lineVao,&lineVbo,previewBasePoint.x,previewBasePoint.y,xPos,yPos,semitoneMapper,colorMapper));
          record();
          lineDrawing = false;
        }

      //select interactable (set interaction mode)
      } else if ( !selected && hovered!=nullptr ) {
        selected = true;
        interactable = hovered;
        interactableCenterDisplacement = ndcToWorldCoordinates(mouseToNDC(mouse)) - interactable->getPosition();
      //I don't remember why we need this...
      } else if ( selected ) {
        selected = false;
        interactable = nullptr;
      }
    }

    /* It feels kinda confusing the way undo redo is doubled mapped over 2 function callbacks */

    //undo 
    if (button == GLFW_MOUSE_BUTTON_4 && action == GLFW_PRESS) {
      undo();
    }

    //redo
    if (button == GLFW_MOUSE_BUTTON_5 && action == GLFW_PRESS) {
      redo();
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
  (void) window;//suppress -Wunused-paramter
  mouse.x = xpos;
  mouse.y = ypos;
}


void drawLinePreview(){

  float xPos; float yPos;
  constrainedPreviewLineTerminal(xPos,yPos);

  auto [name , semitoneMapper, colorMapper ] = scaleData[scaleIndex];
  previewLine->updatePoints(glm::vec2(previewBasePoint.x,previewBasePoint.y),glm::vec2(xPos,yPos));
  previewLine->calculateToneAndColor(semitoneMapper,colorMapper);
  previewLine->draw();
}

/* 
 * place the final coordinate of the preview line after being subjected
 * to the MAX and MIN constraints for lines (these apply to world space)
 */
void constrainedPreviewLineTerminal(float& xPos,float& yPos) {

  auto wscp = ndcToWorldCoordinates(mouseToNDC(mouse));
  auto vec = wscp - previewBasePoint;
  float norm = sqrt(glm::dot(vec,vec));
  glm::vec2 Pos = previewBasePoint;

  if ( norm > keville::util::MAX_LINE_WIDTH ) {
    Pos += (vec/norm * keville::util::MAX_LINE_WIDTH); 
  }

  if ( norm < keville::util::MIN_LINE_WIDTH ) {
    Pos += (vec/norm * keville::util::MIN_LINE_WIDTH); 
  }

  if ( norm > keville::util::MIN_LINE_WIDTH && norm < keville::util::MAX_LINE_WIDTH ) {
    Pos += vec;
  }

  xPos = Pos.x;
  yPos = Pos.y;

}

/*
 * We use the point slope form of a line to test for intersections, if vertical lines are detected, we use that line as the solution for the 
 * x coordinate of the equation. 
 */
bool testLineSegmentIntersection(float xa0,float ya0,float xaf,float yaf,float xb0,float yb0,float xbf,float ybf,float* solx, float* soly) {

  bool aIsVertical = false;
  bool bIsVertical = false;

  float dax = (xaf - xa0);
  float day = (yaf - ya0);
  float ma;
  //divide by zero guard for veritcal lines
  if (dax) { 
    ma  = day / dax;
  } else {
    //return false; 
    aIsVertical = true;
  }

  float dbx = (xbf - xb0);
  float dby = (ybf - yb0);
  float mb;
  //divide by zero guard
  if (dbx) { 
    mb  = dby / dbx;
  } else {
    //return false; 
    bIsVertical = true;
  }

  //solve intersection o two linear equations
  if ( !aIsVertical || !bIsVertical ) {
  
    //using the point slope equation for a line | y - y1 = m (x - x1) 
    //we find a solution to the intersection of our line segments extended to all of the Reals
    
    //divide by zero guard
    if (ma == mb) {
      //this implies coincident lines are treated as not intersecting
      return false; 
    } 

    float solvex;
    float solvey;

    if ( bIsVertical ) {
      solvex = xb0;
    } else if ( aIsVertical ) {
      solvex = xa0;
    } else { 
      solvex = ( (ma * xa0) - (mb * xb0) + yb0 - ya0 );
      solvex/=(ma-mb);
    }
    solvey = ma * (solvex - xa0) + ya0;

    //now we check if solx is in the intersection of our segments x domains
    float axmin = std::min(xa0,xaf);
    float axmax = std::max(xa0,xaf);
    float bxmin = std::min(xb0,xbf);
    float bxmax = std::max(xb0,xbf);

    bool xOk = solvex >= axmin && solvex <= axmax 
      && solvex >= bxmin && solvex <= bxmax;

    if (xOk) {

      //same for y domains
      float aymin = std::min(ya0,yaf);
      float aymax = std::max(ya0,yaf);
      float bymin = std::min(yb0,ybf);
      float bymax = std::max(yb0,ybf);

      bool yOk =  solvey >= aymin && solvey <= aymax 
            && solvey >= bymin && solvey <= bymax;

      if ( yOk ) {
        *solx = solvex;
        *soly = solvey;
        return true;
      }

    }
  }
  *solx = 0;
  *soly = 0;
  return false;
}

void updateSample() {
  std::string name = std::get<0>(sampleData[sampleIndex]);
  unsigned int freq = std::get<1>(sampleData[sampleIndex]);
  std::cout << "loaded new sample : " << name << std::endl;
  soloud.stopAll(); /* undefined behaviour if we load while sample is being played*/
  sample->load(name.c_str());  /*soloud assumes this file will work i.e. not feedback*/
  SAMPLE_BASE_RATE = freq; 
}

void updateScale() {
  auto [name , semitoneMapper, colorMapper ] = scaleData[scaleIndex];
  auto lineItty = lines.begin();
  std::cout << " scale changed, updating lines " << std::endl;
  //adjust all lines so they use the new scales color mapping
  while (lineItty != lines.end()) {
    Line* lp = *lineItty;
    lp->calculateToneAndColor(semitoneMapper,colorMapper);
    lineItty++;
  }
}

float toDegrees(float radians) {
  return radians * 360.0f / (2 * M_PI);
}

void play_bounce_audio(Line* lp) {
  int handle = soloud.play(*sample);
  float pan = lp->getPosition().x / MAX_VIEWPORT_SCALE;
  pan = std::min(pan,1.0f);
  pan = std::max(pan,-1.f);
  soloud.setPan(handle,pan);
  int playback_rate = keville::util::semitone_adjusted_rate(SAMPLE_BASE_RATE,lp->semitone);
  soloud.setSamplerate(handle,playback_rate);
}

//return the hovered interactable if any
Interactable* detect_hover() {

  glm::vec2 mcws = ndcToWorldCoordinates(mouseToNDC(mouse));
  
  for ( auto sp : spawners ) {
    if ( sp->IsHovering(mcws)) {
      return sp;
    }
  }

  for ( auto lp : lines ) {
    if ( lp->IsHovering(mcws)) {
      return lp;
    }
  }
  return nullptr;
}

//the interactable gets moved by the user when selected
void update_interactable() {
  if ( !selected ) return;
  auto mcws = ndcToWorldCoordinates(mouseToNDC(mouse)) - interactableCenterDisplacement;
  interactable->position(mcws.x,mcws.y);
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

    glm::vec2 centerFinal;
    glm::vec2 velocityFinal = bp->getVelocity();//bp->velocity;

    //free falling?
    velocityFinal.y += ballGravity;
    centerFinal = bp->getPosition() + bp->getVelocity();

    //collision with line?
    for ( auto lp : lines ) {
      auto [ pointA , pointB ] = lp->getPoints();
      float solx;
      float soly;

      bool intersect = testLineSegmentIntersection(pointA.x,pointA.y,
          pointB.x,pointB.y,
          bp->getPosition().x,bp->getPosition().y,
          centerFinal.x,centerFinal.y,
          &solx,&soly);

      if (!intersect) {
        continue;
      }

      if (!muteAudio) {
        //  threads must be detached or joined before the exit of the calling scope
        std::thread audio_thread(play_bounce_audio,lp);
        audio_thread.detach();
      }

      //  vertical line case (causes indeterminate line_slope)
      if ( pointB.x == pointA.x ) {
        //  we reflect the velocity vector on the x-axis
        velocityFinal.x *= -1 * collisionRestitution;
        centerFinal.x = bp->getPosition().x + velocityFinal.x;
      //  horizontal line case (causes line_slope to be 0)
      } else if (pointB.y == pointA.y)  {
        //  we reflect the velocity vector on the y-axis
        velocityFinal.y *= -1 * collisionRestitution;
        centerFinal.y = bp->getPosition().y + velocityFinal.y;
      } else {
        //  find the normal vectors
          //  we have two possible normals for our line segment, the right one
          //  will have an obtuse angle the velocity vector, so we check the sign
          //  of the dot product
        float line_slope = (pointB.y - pointA.y) / (pointB.x - pointA.x);
        float normal_slope = -(1/line_slope); //yikes, this won't work when the line is perpendicular
        //  now we have a "normal" vector with <1,normal_slope> but we need to find the right orientation (sign)
        //  compare sign of normal dot velocity
        bool flipped_normal = (1.0f * velocityFinal.x) + (normal_slope * velocityFinal.y) > 0; // ? 
        float normal_x = flipped_normal ? -1 : 1;
        float normal_y = normal_slope * (flipped_normal ? -1 : 1);
        float normal_angle = atan2(normal_y,normal_x); 
        float incline_angle = normal_angle - (M_PI/2);
        //  find the incident angle between normal and the 'flipped' velocity vector
        float normal_dot_velocity = velocityFinal.x * normal_x + velocityFinal.y * normal_y;
        float normal_norm = sqrt(normal_x * normal_x + normal_y * normal_y);
        float velocity_norm = sqrt(velocityFinal.x * velocityFinal.x + velocityFinal.y * velocityFinal.y);
        float incident_angle = acos( normal_dot_velocity / ( normal_norm * velocity_norm ));
        if ( incident_angle > M_PI/2 ) { incident_angle = M_PI - incident_angle; }
        //  do we add or subtract the incident angle to the normal angle?
        float velocity_angle = atan2(velocityFinal.y,velocityFinal.x);
        bool add_incidence = cos(velocity_angle - incline_angle) < 0;
        //  find resultant angle
        float resultant_angle = atan2(normal_y,normal_x) + (add_incidence ? incident_angle  : -incident_angle );
        //  rotate the velocity vector by this angle (this can be rewritten for vectors)
        velocityFinal.x = collisionRestitution * (velocity_norm * cos(resultant_angle));
        velocityFinal.y = collisionRestitution * (velocity_norm * sin(resultant_angle) + ballGravity);
        centerFinal = bp->getPosition() + velocityFinal;
      }
    }

    //set the final state of the ball
    bp->setVelocity(velocityFinal);
    bp->setPosition(centerFinal);

    if (bp->getPosition().y < -MAX_VIEWPORT_SCALE) { 
      delete bp;
      itty = balls.erase(itty);
    } else {
      itty++;
    }

  }

}

//what world space coordinate does this map to?
glm::vec2 ndcToWorldCoordinates(glm::vec2 ndc) {
  //undo ortho proj (scaling)
  glm::vec2 p = glm::vec2(ndc.x*viewportScale,ndc.y*viewportScale);
  //undo translation
  return p - viewportCenter;
}


glm::vec2 mouseToNDC(glm::vec2 mouse) {
  return glm::vec2(
      (2.0f  * mouse.x / (double) windowWidth ) - 1.0f,
      (-2.0f * mouse.y / (double) windowHeight) + 1.0f);
}

//returns the absoulte distance from the origin in the x or y direction that
//the viewport can be centered at given the current scale.
float viewportCenterRange() {
  //return (MAX_VIEWPORT_SCALE - viewportScale)/2.0f;
  return (MAX_VIEWPORT_SCALE - viewportScale);
}

void updateView() {

  //update view uniforms
  lineShader->use();

  int viewLocLine = glGetUniformLocation(lineShader->ID, "View"); 
  glUniformMatrix4fv(viewLocLine, 1, GL_FALSE, glm::value_ptr(view));

  ballShader->use();

  int viewLocBall = glGetUniformLocation(ballShader->ID, "View"); 
  glUniformMatrix4fv(viewLocBall, 1, GL_FALSE, glm::value_ptr(view));

  digitShader->use();

  int viewLocDigit = glGetUniformLocation(digitShader->ID, "View"); 
  glUniformMatrix4fv(viewLocDigit, 1, GL_FALSE, glm::value_ptr(view));

}

//
void changeView(glm::vec2 newCenter) {
    newCenter.x = std::max(-viewportCenterRange(),newCenter.x);
    newCenter.x = std::min(viewportCenterRange(),newCenter.x);
    newCenter.y = std::max(-viewportCenterRange(),newCenter.y);
    newCenter.y = std::min(viewportCenterRange(),newCenter.y);
    viewportCenter = newCenter;
    view = glm::translate(glm::mat4(1),glm::vec3(viewportCenter.x,viewportCenter.y,0));
}

void changeProjection(float scale) {
    viewportScale = std::max(MIN_VIEWPORT_SCALE, scale);
    viewportScale = std::min(MAX_VIEWPORT_SCALE, viewportScale);
    projection = glm::ortho(-viewportScale, viewportScale, -viewportScale, viewportScale, -10.f, 10.0f); //left,right ,bot top , near far
    std::cout << " current viewport scale is " << viewportScale << std::endl;
}

void updateProjection() {

  //update projection uniforms
  lineShader->use();

  int projectionLocLine = glGetUniformLocation(lineShader->ID, "Projection"); 
  glUniformMatrix4fv(projectionLocLine, 1, GL_FALSE, glm::value_ptr(projection));

  ballShader->use();

  int projectionLocBall = glGetUniformLocation(ballShader->ID, "Projection"); 
  glUniformMatrix4fv(projectionLocBall, 1, GL_FALSE, glm::value_ptr(projection));


  digitShader->use();

  int projectionLocDigit = glGetUniformLocation(digitShader->ID, "Projection"); 
  glUniformMatrix4fv(projectionLocDigit, 1, GL_FALSE, glm::value_ptr(projection));

}

void resetViewport() { //set projection and 'view' to defaults
    std::cout << " resetting viewport " << std::endl;
    changeView(glm::vec2(0,0));
    updateView();
    changeProjection(1.0f);
    updateProjection();
}

void record() 
{
  SaveState* now = new SaveState();
  now->save(lines,spawners);
  stateStack.Record(now);
  std::cout << " Recorded " << std::endl;
}

void undo() {
  std::cout << " Undo ! " << std::endl;
  stateStack.Back();
  if ( stateStack.Current() == nullptr ) {
    return;
  }
  stateStack.Current()->load(lines,spawners);
};

void redo() {
  std::cout << " Redo ! " << std::endl;
  stateStack.Forward();
  if ( stateStack.Current() == nullptr ) {
    return;
  }
  stateStack.Current()->load(lines,spawners);
};

void processIO() {
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantCaptureMouse) {
      //do non IMGUI io
    }
    hovered = detect_hover();
    bool interactingBeforeInputs = (interactable != nullptr);  /* for tracking interaction end */
    glfwPollEvents();
    if ( interactable == nullptr && interactingBeforeInputs ) {
      record(); 
    }
    update_interactable();
}


void processGUI() {

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  ImGui::NewFrame();

  if (showConfig) {
    processConfigGui(io);
  }
  if (showHelp) {
    processHelpGui(io);
  }

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

}

void processConfigGui(ImGuiIO& io) {

    /* 
     * this section has alot of doubled variables to enforce variable restrictions
     * I imagine I don't need this duplication of variables & and can constrain directly with
     * ImGUI but this was written when I knew less about it 
     */

    //ImVec2 window_size = ImGui::GetWindowSize();
    //ImVec2 window_pos = ImGui::GetWindowPos();

    ImGuiWindowFlags main_window_flags =  ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse/*do i want this?*/ | ImGuiWindowFlags_AlwaysAutoResize;
    ImGui::SetNextWindowSize({400,600});
    ImGui::SetNextWindowPos(ImVec2(windowWidth-400-10/*windowWidth-400*//*should be window width - size of this - 10*/,50),ImGuiCond_FirstUseEver/*so we move the window*/); 
    ImGui::SetNextWindowBgAlpha(0.35f);
    ImGui::Begin("Options",nullptr,main_window_flags);                         

    int guiSelectedSampleIndex = sampleIndex;
    if(ImGui::CollapsingHeader("Sample")) {
        if (ImGui::BeginListBox("Sample Picker")) {
            for (size_t i = 0; i < sampleData.size(); i++) {
                /* This conversion would be circumvented if sampleData stored paths which is more appropriate */
                const bool alreadySelected = ( (size_t) guiSelectedSampleIndex == i );
                std::filesystem::path samplePath {std::get<0>(sampleData[i])};
                const char* sampleName = samplePath.filename().c_str();
                if (ImGui::Selectable(sampleName, alreadySelected))
                {
                    guiSelectedSampleIndex = i;
                }
            }
          ImGui::EndListBox();
        }
    }

    int guiSelectedScaleIndex = scaleIndex;
    if(ImGui::CollapsingHeader("Scale")) {
       if (ImGui::BeginListBox("Scale Picker") ) {
            for (size_t i = 0; i < scaleData.size(); i++) {
                const bool alreadySelected = ( (size_t) guiSelectedScaleIndex == i );
                auto scale = scaleData[i];
                std::string name =  std::get<0>(scale);
                if (ImGui::Selectable(name.c_str(), alreadySelected))
                {
                    guiSelectedScaleIndex = i;
                }
            }
          ImGui::EndListBox();
        }
    }

    float audioSlider = soloud.getGlobalVolume();
    if(ImGui::CollapsingHeader("Volume")) {
        ImGui::SliderFloat("Master Volume", &audioSlider, GLOBAL_MIN_VOLUME,GLOBAL_MAX_VOLUME, "%.2f", ImGuiSliderFlags_AlwaysClamp);
    }

    float viewportScaleSlider = viewportScale;
    glm::vec2 viewportCenterSlider = viewportCenter;
    float lineThicknessSlider = lineThickness;
    bool resetViewClicked = false;
    if(ImGui::CollapsingHeader("View")) {
        ImGui::SliderFloat("View Scale", &viewportScaleSlider, MIN_VIEWPORT_SCALE, MAX_VIEWPORT_SCALE, "%.2f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("View x", &viewportCenterSlider.x, -viewportCenterRange(), viewportCenterRange(), "%.2f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("View y", &viewportCenterSlider.y, -viewportCenterRange(), viewportCenterRange(), "%.2f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("Line Thickness", &lineThicknessSlider, MIN_LINE_THICKNESS, MAX_LINE_THICKNESS, "%.4f", ImGuiSliderFlags_AlwaysClamp);
        if(ImGui::Button("Reset")) {
            resetViewClicked = true;
        }
    }

    static int ballGravityScaleRadio = ballGravityScale;
    if(ImGui::CollapsingHeader("Gravity")) {
    ImGui::RadioButton("1/4x", &ballGravityScaleRadio, -2); ImGui::SameLine();
    ImGui::RadioButton("1/2x", &ballGravityScaleRadio, -1); ImGui::SameLine();
    ImGui::RadioButton("1x",   &ballGravityScaleRadio, 0); ImGui::SameLine();
    ImGui::RadioButton("2x",   &ballGravityScaleRadio, 1); ImGui::SameLine();
    ImGui::RadioButton("4x",   &ballGravityScaleRadio, 2); 
    } 

    int newSaveSlot = selectedSaveSlot;
    bool saveClicked = false;
    bool loadClicked = false;
    if(ImGui::CollapsingHeader("Save/Load")) {
        for ( int n = 0; n < (int) NUM_SAVE_SLOTS; n++ ) {
            char buf[32];
            sprintf(buf,"State %d",n);
            if ( ImGui::Selectable(buf, newSaveSlot == n ))
                selectedSaveSlot = n;
        }
        ImGui::SeparatorText("");
        if(ImGui::Button("Save")) {
            saveClicked = true;
        }
        ImGui::SameLine();
        if(ImGui::Button("Load")) {
            loadClicked = true;
        }
    }

    if(ImGui::CollapsingHeader("Files")) {
      static char saveFilePathString[64];
      ImGui::InputText("File",saveFilePathString,64);
      ImGui::SameLine();
      if(ImGui::Button("Save")) {
        std::cout << " save clicked for path :" << saveFilePathString << std::endl;
        saveToFile(lines,spawners,std::string{saveFilePathString});
      }


      if(ImGui::Button("load")) {
        std::cout << " load clicked for path :" <<selectedLoadFile.string() << std::endl; 
        loadFromFile(lines,spawners,selectedLoadFile);
      }
      ImGui::SameLine();
      if (ImGui::BeginListBox("load file picker")) {
        for ( const std::filesystem::path& p : std::filesystem::directory_iterator(DATA_PATH) ) {
          bool selected = selectedLoadFile.string() == p.string();
          if (ImGui::Selectable(p.filename().string().c_str(),selected)) {
            selectedLoadFile = p;
          }
        }
        ImGui::EndListBox();
      }
    }

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    ImGui::Text("Active Voices : %u", soloud.getActiveVoiceCount());
    ImGui::Text("Balls : %li", balls.size());

    ImGui::End();


    //Process State

    //If ImGui ListBox and ComboBox sets a negative index, we have a invalid index...
    if ( guiSelectedSampleIndex >= 0 && (unsigned int) guiSelectedSampleIndex != sampleIndex) {
        sampleIndex = guiSelectedSampleIndex;
        updateSample();
    }
    if (guiSelectedScaleIndex >= 0 && (unsigned int) guiSelectedScaleIndex != scaleIndex) {
        scaleIndex = guiSelectedScaleIndex;
        updateScale();
    }

    if ( audioSlider != soloud.getGlobalVolume() ) {
        audioSlider = std::max(GLOBAL_MIN_VOLUME,audioSlider);
        audioSlider = std::min(GLOBAL_MAX_VOLUME,audioSlider);
        soloud.setGlobalVolume(audioSlider);
    }


    // View Port

    bool scaleChanged = false;
    if ( viewportScaleSlider != viewportScale ) {
        scaleChanged = true;
        changeProjection(viewportScaleSlider);
        updateProjection();
    }

    if ( scaleChanged || viewportCenterSlider != viewportCenter ) {
        changeView(viewportCenterSlider);
        updateView();
    }

    if ( lineThickness != lineThicknessSlider ) {
        lineShader->use();
        lineThickness = std::max(MIN_LINE_THICKNESS,lineThicknessSlider);
        lineThickness = std::min(MAX_LINE_THICKNESS,lineThicknessSlider);
        //update Thicken matrix
        thicken = glm::scale(glm::mat4(1),glm::vec3(1,lineThickness,1));
        //update uniform
        int thickenLocLine = glGetUniformLocation(lineShader->ID, "Thicken"); 
        glUniformMatrix4fv(thickenLocLine, 1, GL_FALSE, glm::value_ptr(thicken));
    }



    if ( resetViewClicked ) {
        resetViewport();
    }


    if (ballGravityScaleRadio != ballGravityScale) {
      ballGravityScale = ballGravityScaleRadio;
      ballGravity = GRAVITY_BASE * (pow(2,ballGravityScale));
      std::cout << " New " << ballGravity << std::endl;
    }

    if ( saveClicked ) {
        //save to selectd slot
        std::cout << " saving to slot " << selectedSaveSlot << std::endl;
        saveSlots[selectedSaveSlot].save(lines,spawners);
    }

    if ( loadClicked ) {
        //load to selectd slot
        std::cout << " loading slot " << selectedSaveSlot << std::endl;
        saveSlots[selectedSaveSlot].load(lines,spawners);
        stateStack.Reset();
    }

}

void processHelpGui(ImGuiIO& io) {

  ImVec2 window_size = ImGui::GetWindowSize();
  ImVec2 window_pos = ImGui::GetWindowPos();
  ImGuiWindowFlags window_flags =  ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;

  ImGui::SetNextWindowSize({230,600});
  ImGui::SetNextWindowPos(ImVec2(10,50),ImGuiCond_FirstUseEver/*so we move the window*/); 
  ImGui::SetNextWindowBgAlpha(0.35f);

  ImGui::Begin("test overlay", nullptr, window_flags);

  //future me would prefer all these keystrokes map to an event structure
  //an that structure takes a key an optionally a meta key ex. Alt
  //then when setting up the keystroke callback we functionally evaluate this
  //variable keymapping, similary we can iterate over it here, instead of 2 explicit lists

  ImGui::SeparatorText("View");
  ImGui::BeginTable("View",2);


  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::Text("Pan Left");
  ImGui::TableSetColumnIndex(1);
  ImGui::Text("\'Left\'");

  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::Text("Pan Right");
  ImGui::TableSetColumnIndex(1);
  ImGui::Text("\'Right\'");

  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::Text("Pan Up");
  ImGui::TableSetColumnIndex(1);
  ImGui::Text("\'Up\'");

  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::Text("Pan Down"); 
  ImGui::TableSetColumnIndex(1);
  ImGui::Text("\'Down\'");

  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::Text("Zoom out"); 
  ImGui::TableSetColumnIndex(1);
  ImGui::Text("\'Alt\' \'Up\'");

  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::Text("Zoom in"); 
  ImGui::TableSetColumnIndex(1);
  ImGui::Text("\'Alt\' \'Down\'");

  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::Text("Reset view"); 
  ImGui::TableSetColumnIndex(1);
  ImGui::Text("\'r\'");

  ImGui::EndTable();

  ImGui::SeparatorText("Normal Mode");
  ImGui::BeginTable("Normal Mode",2);

  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::Text("Show/Hide Config");
  ImGui::TableSetColumnIndex(1);
  ImGui::Text("\'c\'");

  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::Text("Show/Hide Help");
  ImGui::TableSetColumnIndex(1);
  ImGui::Text("\'h\'");

  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::Text("Select");
  ImGui::TableSetColumnIndex(1);
  ImGui::Text("\'Mouse1\'");

  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::Text("Start/End draw");
  ImGui::TableSetColumnIndex(1);
  ImGui::Text("\'Mouse1\'");

  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::Text("Pause");
  ImGui::TableSetColumnIndex(1);
  ImGui::Text("\'p\'");

  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::Text("Mute");
  ImGui::TableSetColumnIndex(1);
  ImGui::Text("\'m\'");

  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::Text("redo");
  ImGui::TableSetColumnIndex(1);
  ImGui::Text("\'L_CONTROL\' \'r\'");

  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::Text("redo");
  ImGui::TableSetColumnIndex(1);
  ImGui::Text("\'Mouse4\'");

  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::Text("undo");
  ImGui::TableSetColumnIndex(1);
  ImGui::Text("\'u\'");

  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::Text("undo");
  ImGui::TableSetColumnIndex(1);
  ImGui::Text("\'Mouse5\'");

  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::Text("new spawner");
  ImGui::TableSetColumnIndex(1);
  ImGui::Text("\'s\'");

  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::Text("quit");
  ImGui::TableSetColumnIndex(1);
  ImGui::Text("\'q\'");

  ImGui::EndTable();


  ImGui::SeparatorText("Interaction Mode");
  ImGui::BeginTable("Interaction Mode",2);

  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::Text("increase spawner rate");
  ImGui::TableSetColumnIndex(1);
  ImGui::Text("\'Up\'");

  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::Text("decrease spawner rate");
  ImGui::TableSetColumnIndex(1);
  ImGui::Text("\'Down\'");

  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::Text("delete");
  ImGui::TableSetColumnIndex(1);
  ImGui::Text("\'d\'");

  ImGui::EndTable();
  ImGui::End();

}

void draw() {

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

}

void init() {

  //initialize soloud
  soloud.init();
  soloud.setGlobalVolume(GLOBAL_DEFAULT_VOLUME);
  soloud.setMaxActiveVoiceCount(MAX_VOICE_COUNT);
  sample = new SoLoud::Wav;
 
  // These names gotta change..
  generateSampleData();
  generateScaleData();


  //initialize glfw
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_SAMPLES, MSAA);
  window = glfwCreateWindow(800, 600, "Drop Sound", NULL, NULL);

  if ( window == NULL ) 
  {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    exit(-1);
  }
  glfwMakeContextCurrent(window);

  //we must register these before ImGui init
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetKeyCallback(window, key_callback);

  glfwSetCursorPosCallback(window, mouse_callback);

  if (vsync) {
    glfwSwapInterval(vsync);
  }

  //create IMGUI context
  const char* glsl_version = "#version 330"; //IMGUI needs to know the glsl version
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  io.IniFilename = nullptr; //prevent imgui from generating a config file
  ImGui_ImplGlfw_InitForOpenGL(window, true);                 // configure backends
  ImGui_ImplOpenGL3_Init(glsl_version);
  ImGui::StyleColorsDark();

  //initialize GLAD (to locate opengl call memory addresses)

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) 
  {
    std::cout << "Failed to init GLAD" << std::endl;
    exit(-2);
  }

  glEnable(GL_MULTISAMPLE);//MSAA
  glViewport(0, 0, 800, 600);
  glfwGetWindowSize(window,&windowWidth,&windowHeight);
  glClearColor(clearColor.x,clearColor.y,clearColor.z,clearColor.w);


  //load digit textures
  stbi_set_flip_vertically_on_load(true);
  glGenTextures(10,digitTextures);
  for ( int i = 0; i < 10; i++ ) {
    int width, height, nChannels;
    std::string path = RES_PATH+"/textures/digits/digit" + std::to_string(i) + ".png";
    unsigned char *data = stbi_load(path.c_str(), &width, &height, &nChannels, 0);
    if ( ! data ) {
      std::cout << "error loading texture " << path << std::endl;
    } else {
      glBindTexture(GL_TEXTURE_2D, digitTextures[i]);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
      glGenerateMipmap(GL_TEXTURE_2D);
    }
  }

  //construct shader programs

  digitShader = new Shader((RES_PATH+"/shaders/digit.vs").c_str(),(RES_PATH+"/shaders/digit.fs").c_str());
  lineShader = new Shader((RES_PATH+"/shaders/line.vs").c_str(),(RES_PATH+"/shaders/line.fs").c_str());
  ballShader = new Shader((RES_PATH+"/shaders/ball.vs").c_str(),(RES_PATH+"/shaders/line.fs").c_str());

  //digitShader
  float digitVertexData[30] = {
    0.f,0.0f,0.f, /*Pos*/ 0.0f,0.0f, /*Tex*/
    1.f,0.0f,0.f,         1.0f,0.0f,
    1.f,1.0f,0.f,         1.0f,1.0f,

    1.f,1.0f,0.f,         1.0f,1.0f,
    0.f,1.0f,0.f,         0.0f,1.0f,
    0.f,0.0f,0.f,         0.0f,0.0f
  };
  std::copy(digitVertexData,digitVertexData+30,digitVertices);

  glGenVertexArrays(1, &digitVao);
  glGenBuffers(1,&digitVbo);
  //assemble vertex array
  glBindBuffer(GL_ARRAY_BUFFER,digitVbo);
  glBindVertexArray(digitVao);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0); 
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3*sizeof(float)));
  glEnableVertexAttribArray(1); 
  //initialize vertex buffer
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 5, digitVertices, GL_STATIC_DRAW);


  // Line Rendering Vars
  /*
   * a square centered vertically on the x axis 
   * that is aligned with the y axis
   */
  float lineVertexData[18] = {
    0.f,1.0f,0.f,
    1.f,1.0f,0.f,
    1.f,-1.0f,0.f,

    1.f,-1.0f,0.f,
    0.f,-1.0f,0.f,
    0.f,1.0f,0.f,
  };
  std::copy(lineVertexData,lineVertexData+18,lineVertices);

  glGenVertexArrays(1, &lineVao);
  glGenBuffers(1,&lineVbo);
  //assemble vertex array
  glBindBuffer(GL_ARRAY_BUFFER,lineVbo);
  glBindVertexArray(lineVao);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0); 
  glBindVertexArray(0);
  //initialize vertex buffer
  glBindBuffer(GL_ARRAY_BUFFER, lineVbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 3, lineVertices, GL_STATIC_DRAW);

  // Balll Rendering Vars
  int ballVertexTotal; /*this is useless, below method does not need this*/
  ballVertices =  keville::util::generate_regular_polygon_vertices(keville::util::CIRCLE_SIDES,1,ballVertexTotal);
  //generate buffers
  glGenVertexArrays(1, &ballVao);
  glGenBuffers(1,&ballVbo);
  //assemble vertex array
  glBindBuffer(GL_ARRAY_BUFFER,ballVbo);
  glBindVertexArray(ballVao);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0); 
  glBindVertexArray(0);
 
  //initialize vertex buffer
  glBindBuffer(GL_ARRAY_BUFFER, ballVbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * 3 * keville::util::CIRCLE_SIDES, ballVertices, GL_STATIC_DRAW);//stores 2 lines
                                                                                                                  

  // Spawner Rendering Vars

  int spawnerVertexTotal = 0; /*this is useless, below method does not need this*/
  spawnerVertices = keville::util::generate_regular_polygon_hull_vertices(keville::util::CIRCLE_SIDES,1,spawnerVertexTotal);

  //generate buffers
  glGenVertexArrays(1, &spawnerVao);
  glGenBuffers(1,&spawnerVbo);
  //assemble vertex array
  glBindBuffer(GL_ARRAY_BUFFER,spawnerVbo);
  glBindVertexArray(spawnerVao);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0); 
  glBindVertexArray(0);
 
  //initialize vertex buffer
  glBindBuffer(GL_ARRAY_BUFFER, spawnerVbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * 2 * keville::util::CIRCLE_SIDES, spawnerVertices, GL_STATIC_DRAW);

  //set default uniforms for shader programs

  lineShader->use();
  int projectionLocLine = glGetUniformLocation(lineShader->ID, "Projection"); 
  int viewLocLine = glGetUniformLocation(lineShader->ID, "View"); 
  int thickenLocLine = glGetUniformLocation(lineShader->ID, "Thicken"); 

  glUniformMatrix4fv(projectionLocLine, 1, GL_FALSE, glm::value_ptr(projection));
  glUniformMatrix4fv(viewLocLine, 1, GL_FALSE, glm::value_ptr(view));
  glUniformMatrix4fv(thickenLocLine, 1, GL_FALSE, glm::value_ptr(thicken));

  ballShader->use();
  int projectionLocBall = glGetUniformLocation(ballShader->ID, "Projection"); 
  int viewLocBall = glGetUniformLocation(ballShader->ID, "View"); 

  glUniformMatrix4fv(projectionLocBall, 1, GL_FALSE, glm::value_ptr(projection));
  glUniformMatrix4fv(viewLocBall, 1, GL_FALSE, glm::value_ptr(view));


  digitShader->use();
  int projectionLocDigit = glGetUniformLocation(digitShader->ID, "Projection"); 
  int viewLocDigit = glGetUniformLocation(digitShader->ID, "View"); 
  int colorLocDigit = glGetUniformLocation(digitShader->ID, "Color"); 

  glUniformMatrix4fv(projectionLocDigit, 1, GL_FALSE, glm::value_ptr(projection));
  glUniformMatrix4fv(viewLocDigit, 1, GL_FALSE, glm::value_ptr(view));
  glUniform3f(colorLocDigit,1.0f,0.0f,1.0f);


  // Create default object instances

  auto [name , semitoneMapper, colorMapper ] = scaleData[scaleIndex];
  auto wscp = ndcToWorldCoordinates(mouseToNDC(mouse));

  previewLine = new Line(lineShader,&lineVao,&lineVbo,previewBasePoint.x,previewBasePoint.y,wscp.x,wscp.y,semitoneMapper,colorMapper);
  spawners.push_back(new Spawner( /*spawner shader is ball shader*/ballShader, &spawnerVao, &spawnerVbo,
                                  digitShader, &digitVao, &digitVbo, digitTextures,
                                  ballShader, &ballVao,&ballVbo,
                                  DEFAULT_SPAWN_X,DEFAULT_SPAWN_Y,
                                  DEFAULT_BASE_SPAWN_FREQUENCY,2.0f));


}

void update() {

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

    if (!pausePhysics) {
      update_balls();
    }

}

void generateScaleData() {
  // musical scales
  scaleData.push_back({"Major",
      [] (float width) {
        return keville::util::line_width_to_semitone_linear_major(width,12); 
      },
      [] (int semitones) {
        return keville::util::semitone_color_chromatic(semitones);
      }});
  scaleData.push_back({"Major Pentatonic",
      [] (float width) {
        return keville::util::line_width_to_semitone_linear_major_pentatonic(width,12); 
      },
      [] (int semitones) {
        return keville::util::semitone_color_chromatic(semitones);
      }});
  scaleData.push_back({"Minor",
      [] (float width) {
        return keville::util::line_width_to_semitone_linear_minor(width,12); 
      },
      [] (int semitones) {
        return keville::util::semitone_color_chromatic(semitones);
      }});
  scaleData.push_back({"Minor Pentatonic",
      [] (float width) {
        return keville::util::line_width_to_semitone_linear_minor_pentatonic(width,12); 
      },
      [] (int semitones) {
        return keville::util::semitone_color_chromatic(semitones);
      }});
  scaleData.push_back({"Blues",
      [] (float width) {
        return keville::util::line_width_to_semitone_linear_blues(width,12); 
      },
      [] (int semitones) {
        return keville::util::semitone_color_chromatic(semitones);
      }});
  scaleData.push_back({"Chromatic",
      [] (float width) {
        return keville::util::line_width_to_semitone_linear_chromatic(width,12); 
      },
      [] (int semitones) {
        return keville::util::semitone_color_chromatic(semitones);
      }});
  scaleIndex = 0;
  updateScale();
}



/* not a great method name, nor is sampleData indicative of what it
 * stores i.e sample file paths & frequencies ...
 */
void generateSampleData() {

  using namespace std::filesystem;

  path res {RES_PATH+"/audio"};

  try {
    if (is_directory(res)) {
      for ( const path& x : directory_iterator{res}) {
          if ( is_regular_file(x) && x.extension() == ".wav") {
            if (validateWavFile(x)) {
              sampleData.push_back({x.string(),44100});
            }
          }
      }
    }
  } catch (const filesystem_error& err) {
    std::cout << "Error Loading Samples " << std::endl;
  }

  if (!sampleData.size()) shutdown();
  sampleIndex = 0;
  updateSample();

}

/* soloud provides no means of validating a sample loaded
 * with sample.load, therefore it's up to me to figure out
 * if the data i'm trying to load is valid 
 */
bool validateWavFile(std::filesystem::path) {
  /* stub for implementation */
  return true;
}

void shutdown() {
  glfwTerminate();
  soloud.deinit();
}

void saveToFile(std::vector<Line*>& lines, std::vector<Spawner*>& spawners,std::string fileName) {
  std::filesystem::path path {DATA_PATH};
  if (!std::filesystem::exists(DATA_PATH)) {
    std::cout << " DATA_PATH : " << DATA_PATH << " does not exist , creating ... " << std::endl;
    std::filesystem::create_directory(DATA_PATH); 
  }

  //Is file name safe (no escaping this directory)
  std::regex anyParentLinux {R"(.*\.\.\/.*)"};   //any occurence of ../
  std::regex anyParentWindows {R"(.*\.\.\/.*)"}; //any occurence of ..\

  if (std::regex_match(fileName, anyParentLinux) || std::regex_match(fileName,anyParentWindows)) {
    std::cout << " illegal file name " << std::endl;
    std::cout << " aborting save" << std::endl;
    return;
  }

  path/=fileName;

  std::ofstream ofs {path};
  if (!ofs) {
    std::cerr << " error saving state to file path :" << path << std::endl;
    return;
  }
  std::cout << " saving to file " << path << std::endl;
  ofs << "lines" << std::endl;
  for ( auto lp : lines ) {
    ofs << *lp << std::endl;
  }
  ofs << "spawners" << std::endl;
  for ( auto sp : spawners ) {
    ofs << *sp << std::endl;
  }
}

/* load lines and balls data from a file, obliterate the current lines & balls */
void loadFromFile(std::vector<Line*>& lines, std::vector<Spawner*>& spawners,std::filesystem::path path) {

  if (!std::filesystem::exists(path)) {
    std::cerr << " No such file " << path << std::endl;
    return;
  }
  std::ifstream ifs {path};
  if (!ifs) {
    std::cerr << " error loading state from file :" << path << std::endl;
    return;
  }

  //clear extant data
  for ( auto lp : lines ) { 
    delete lp;
  }
  for ( auto sp : spawners ) { 
    delete sp;
  }
  lines.clear();
  spawners.clear();

  //parse & generate lines 

  std::string istring;
  if (!getline(ifs,istring) || istring != "lines" ) {
    std::cerr << " bad file format " <<std::endl;
    return;
  }
  while(getline(ifs,istring) && istring != "spawners" ) {
    char junk;
    float ax, ay, bx, by;
    std::stringstream lineData(istring);
    lineData >> junk;
    lineData >> ax;
    lineData >> junk;
    lineData >> ay;
    lineData >> junk;
    lineData >> bx;
    lineData >> junk;
    lineData >> by;
    //std::cout << " ax : " << ax << " ay : " << ay << "bx : " << bx << "by : " << by << std::endl;
    auto [name , semitoneMapper, colorMapper ] = scaleData[scaleIndex];
    lines.push_back(new Line(lineShader,&lineVao,&lineVbo,ax,ay,bx,by,semitoneMapper,colorMapper));
  }

  //parse & generate spawners

  if ( istring != "spawners" ) {
    std::cerr << " bad file format " <<std::endl;
    return;
  }
  
  while(getline(ifs,istring)) {
    std::stringstream lineData(istring);
    char junk;
    float ax;
    float ay;
    unsigned int scale;
    lineData >> junk;
    lineData >> ax;
    lineData >> junk;
    lineData >> ay;
    lineData >> junk;
    lineData >> scale;
    //std::cout << " ax : " << ax << " ay : " << ay << " scale " << scale << std::endl;
    spawners.push_back(new Spawner( ballShader, &spawnerVao, &spawnerVbo,
                                    digitShader, &digitVao, &digitVbo, digitTextures,
                                    ballShader, &ballVao,&ballVbo,
                                    ax,ay,
                                    DEFAULT_BASE_SPAWN_FREQUENCY,scale));
  }
}






























