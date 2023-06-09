//std
#include <iostream>
#include <cmath>
#include <vector>
#include <thread>
#include <tuple> 
#include <functional>
//Third Party
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "soloud.h"
#include "soloud_wav.h"
#include "soloud_thread.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
//keville
#include "shader.cpp"
#include "Line.h"
#include "Ball.h"
#include "util.h"
#include "Spawner.h"
#include "Interactable.h"
#include "SaveState.h"

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

//line preview var
float preview[2*3]{};           
unsigned int drawPreviewVao;
unsigned int drawPreviewVbo;

//balls
double ballGravity =  -0.0002f;
double collisionRestitution = 0.95f;
std::vector<Ball*> balls;

double DEFAULT_SPAWN_X = -0.5f;
double DEFAULT_SPAWN_Y = 0.5f;

float DEFAULT_BASE_SPAWN_FREQUENCY = (1.0f/3.0f);
float DEFAULT_SPAWN_SCALE = 1.0f;
float SPAWNER_SCALE_MAX = 5.0f;
float SPAWNER_SCALE_MIN = 1.0f;
std::vector<Spawner*> spawners;

//Interactable (the selected entity)
Interactable* hovered      = nullptr;
Interactable* interactable = nullptr; 

//Audio
SoLoud::Soloud soloud;
SoLoud::Wav* sample;
unsigned int sampleIndex = 0;
std::vector<std::tuple<std::string,unsigned int>> sampleData; //sampleName, sampleRate
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

const unsigned int NUM_SAVE_SLOTS = 3;

//save slots
int selectedSaveSlot = 0;
SaveState saveSlots[NUM_SAVE_SLOTS];
                                                              

//Key states
bool S_PRESSED = false;
bool P_PRESSED = false;
bool M_PRESSED = false;
bool UP_PRESSED = false;
bool DOWN_PRESSED = false;

//glfw callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
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
//application utilities
float toDegrees(float radians);
double mouseXToViewX(double mousex);
double mouseYToViewY(double mousey);


int main() {

  //initialize soloud
  soloud.init();
  soloud.setGlobalVolume(GLOBAL_DEFAULT_VOLUME);
  soloud.setMaxActiveVoiceCount(MAX_VOICE_COUNT);
  //soloud.setMaxActiveVoiceCount(32);
  // audio samples
  sampleData.push_back({"res/marimba.wav",44100});
  sampleData.push_back({"res/clipped/sine_440hz_44100_100ms.wav",44100});
  //new (not sure if sample rates are accurate)
  sampleData.push_back({"res/bass-d.wav",44100});
  sampleData.push_back({"res/break-glass.wav",44100});
  sampleData.push_back({"res/chime-a.wav",44100});
  sampleData.push_back({"res/emu.wav",44100});
  sampleData.push_back({"res/kalimba.wav",44100});
  sampleData.push_back({"res/laz6.wav",44100});
  sampleData.push_back({"res/sneeze.wav",44100});
  sampleData.push_back({"res/toy-piano-g.wav",44100});
  sampleData.push_back({"res/xylophone.wav",44100});
  sampleIndex = 0;
  updateSample();

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

  //we must register these before ImGui init
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetCursorPosCallback(window, mouse_callback);

  //create IMGUI context
  const char* glsl_version = "#version 330"; //IMGUI needs to know the glsl version
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  ImGui_ImplGlfw_InitForOpenGL(window, true);               //configure backends
  ImGui_ImplOpenGL3_Init(glsl_version);
  ImGui::StyleColorsDark();

  //initialize GLAD (to locate opengl call memory addresses)

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) 
  {
    std::cout << "Failed to init GLAD" << std::endl;
    return -2;
  }

  glViewport(0, 0, 800, 600);
  glfwGetWindowSize(window,&windowWidth,&windowHeight);

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

  spawners.push_back(new Spawner(ballShader,ballShader,
        DEFAULT_SPAWN_X,DEFAULT_SPAWN_Y,
        DEFAULT_BASE_SPAWN_FREQUENCY,4.0f));
  
  while(!glfwWindowShouldClose(window))
  {

    //start the dear ImGUI frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ///////////////////////////////
    //INPUT
    ///////////////////////////////
    
    hovered = detect_hover();
    if (!io.WantCaptureMouse) {
      processInput(window);
    }
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


    ImGui::SetNextWindowPos(ImVec2(windowWidth-(windowWidth/6),40.0f),ImGuiCond_FirstUseEver); //Enum FirstUseEver allows us to move this window (otherwise it's locked)
    ImGui::SetNextWindowSize(ImVec2(400.0f,20.0f),ImGuiCond_FirstUseEver); //likewise, we can resize with the enum 
    ImGui::Begin("Audio Sample Loader");                         

    int guiSelectedSampleIndex = sampleIndex;
    const char* sampleNames[sampleData.size()];
    for ( size_t i = 0; i < sampleData.size(); i++ ) {
      sampleNames[i] = std::get<0>(sampleData[i]).c_str();
    }

    if(ImGui::CollapsingHeader("Audio Sample Loader")) {
      ImGui::ListBox("Sample", &guiSelectedSampleIndex, sampleNames , IM_ARRAYSIZE(sampleNames), 5);
    }

    int guiSelectedScaleIndex = scaleIndex;
    const char* scaleNames[scaleData.size()] = {} ;
    for ( size_t i = 0; i < scaleData.size(); i++ ) {
      scaleNames[i] = std::get<0>(scaleData[i]).c_str();
    }

    if(ImGui::CollapsingHeader("Scale Picker")) {
      ImGui::ListBox("Scale", &guiSelectedScaleIndex, scaleNames , IM_ARRAYSIZE(scaleNames), 5);
    }

    float audioSlider = soloud.getGlobalVolume();
    if(ImGui::CollapsingHeader("Volume")) {
      ImGui::SliderFloat("Master Volume", &audioSlider, GLOBAL_MIN_VOLUME,GLOBAL_MAX_VOLUME, "%.2f", ImGuiSliderFlags_AlwaysClamp);
    }

    int newSaveSlot = selectedSaveSlot;
    bool saveClicked = false;
    bool loadClicked = false;
    if(ImGui::CollapsingHeader("Save/Load")) {
      for ( int n = 0; n < NUM_SAVE_SLOTS; n++ ) {
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

    if(ImGui::CollapsingHeader("Debug")) {
      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
      ImGui::Text("Active Voices : %u", soloud.getActiveVoiceCount());
      ImGui::Text("Balls : %li", balls.size());
   }


    ImGui::End();

    ////////////////////
    //Render ImGUI
    ////////////////////
    
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    ////////////////////
    //Process ImGui state
    ////////////////////
   
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

    if ( saveClicked ) {
      //save to selectd slot
      std::cout << " saving to slot " << selectedSaveSlot << std::endl;
      saveSlots[selectedSaveSlot].save(lines,spawners);
    }

    if ( loadClicked ) {
      //load to selectd slot
      std::cout << " loading slot " << selectedSaveSlot << std::endl;
      saveSlots[selectedSaveSlot].load(lines,spawners);
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
    } }
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
    //if ImGui window "Wants" the mouse, we do not process below
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
      return;
    }

    (void) window;//suppress -Wunused-paramter
    (void) mods;//suppress -Wunused-paramter
    
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
            //get mapping functions for the line instance
          auto [name , semitoneMapper, colorMapper ] = scaleData[scaleIndex];
          lines.push_back(new Line(lineShader,preview[0],preview[1],xPos,yPos,semitoneMapper,colorMapper));
          lineDrawing = false;

        }

      ////////////////////////////////
      //interactable selection 
      ////////////////////////////////
      } else if ( !selected && hovered!=nullptr ) {
        selected = true;
        interactable = hovered;

      } else if ( selected ) {
        selected = false;
        interactable = nullptr;
      }

    }

}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
  (void) window;//suppress -Wunused-paramter
  mouseX = xpos;
  mouseY = ypos;
}


void drawLinePreview(){
  float xPos;
  float yPos;
  constrainedPreviewLineTerminal(xPos,yPos);

  preview[3] = xPos;
  preview[4] = yPos;
  preview[5] = 0.0f;

  float width = sqrt( pow(preview[4] - preview[1],2) + pow(preview[3] - preview[0],2));

  auto [name , semitoneMapper, colorMapper ] = scaleData[scaleIndex];
  int semitone = semitoneMapper(width);
  std::tuple<float,float,float> color = colorMapper(semitone);

  lineShader->use();
  int ColorLoc = glGetUniformLocation(lineShader->ID, "Color"); 
  glBindVertexArray(drawPreviewVao); 
  glBindBuffer(GL_ARRAY_BUFFER, drawPreviewVbo);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(preview), preview);
  glUniform3f(ColorLoc,std::get<0>(color),std::get<1>(color),std::get<2>(color));
  glLineWidth(6.0f);
  glDrawArrays(GL_LINES, 0, 2);
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

/*
 * This aproach utilizes matrix theory and models the problem of intersection with the matrix equation Ax = b,
 * our point of intersection is precisely Ainv b, which we then check the bounds of.
 *
 * To utilize the matrix equation we need to find a representation of our lines in general form.
 * The approach here is to use the point slope form to find the coefficients of the general form.
 * When the slope of our line is undefined, we skip this since the general form is trivial as x = c
 *
 * UPDATE : This approach suffers the same fundamental issue above, when vertical lines are present , the 'system' is inconsistent
 * the determinant is 0 and the solution does not exist.
 *
 */
/*
bool testLineSegmentIntersection(float x00,float y00,float x0f,float y0f,float x10,float y10,float x1f,float y1f,float* solx, float* soly) {

  //find the coefficients of general form for lines A and B

  // a slope
  float d0x = (x0f - x00);
  float d0y = (y0f - y00);
  float m0;

  // general form coefficients Ax + By + C = 0
  float a0 = 0;
  float b0 = 0;
  float c0 = 0;

  if (d0x) { 
    m0  = d0y / d0x;
    a0 = -1*m0;
    b0 = 1;
    c0 = m0*x00 + y00;
  } else { 
    //vertical line
    c0 = x00; 
  }

  //b slope
  float d1x = (x1f - x10);
  float d1y = (y1f - y10);
  float m1;

  // general form coefficients Ax + By + C = 0
  float a1 = 0;
  float b1 = 0;
  float c1 = 0;

  if (d1x) { 
    m1  = d1y / d1x;
    a1 = -1*m1;
    b1 = 1;
    c1 = m1*x10 + y10;
  } else {
    //vertical line
    c1 = x10;
  }

  // calculate the determinant of matrix A
  float determinant = (a0 * b1) - (b0 * a1);

  float solvex = (c1*b0) - (c0*b1) / determinant;
  float solvey = (c0*a1) - (c1*a0) / determinant;

  //is solution in the bounds of these line segments?
  float xmin0 = std::min(x00,x0f);
  float xmax0 = std::max(x00,x0f);
  float xmin1 = std::min(x10,x1f);
  float xmax1 = std::max(x10,x1f);

  if ( solvex > xmin0 && solvex < xmax0
        && solvex > xmin1 && solvex < xmax1 ) {
    *solx = solvex;
    *soly = solvey;
    return true;
  }

  solx = 0;
  soly = 0;
  return false;
}
*/

void updateSample() {
  std::string name = std::get<0>(sampleData[sampleIndex]);
  unsigned int freq = std::get<1>(sampleData[sampleIndex]);
  std::cout << "loaded new sample : " << name << std::endl;
  delete sample;
  sample = new SoLoud::Wav;
  sample->load(name.c_str()); 
  //adjust all lines so that they use the new sample's frequency
  SAMPLE_BASE_RATE = freq; }

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

//transform mouseX to a view plane coordinate (-1.0,1.0)
double mouseXToViewX(double mousex) {
  return (2.0f  * mousex / (double) windowWidth  ) - 1.0f;
}

//transform mouseY to a view plane coordinate (-1.0,1.0)
double mouseYToViewY(double mousey) {
  return (-2.0f * mousey / (double) windowHeight ) + 1.0;
}

void play_bounce_audio(Line* lp) {
  int handle = soloud.play(*sample);
  int playback_rate = keville::util::semitone_adjusted_rate(SAMPLE_BASE_RATE,lp->semitone);
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
        std::thread audio_thread(play_bounce_audio,lp);
        audio_thread.detach();
      }

      //vertical line case (causes indeterminate line_slope)
      if ( lvert[3] == lvert[0] ) {
        //we just reflect the velocity vector on the x-axis
        vxf *= -1 * collisionRestitution;
        cxf = bp->cx + vxf;
      //horizontal line case (causes line_slope to be 0)
      } else if (lvert[4] == lvert[1])  {
        //we just reflect the velocity vector on the y-axis
        vyf *= -1 * collisionRestitution;
        cyf = bp->cy + vyf;
      } else {
        //find the normal vectors
        //we have two possible normals for our line segment, the right one
        //will have an obtuse angle the velocity vector, so we check the sign
        //of the dot product
        float line_slope = (lvert[4] - lvert[1]) / (lvert[3] - lvert[0]);
        float normal_slope = -(1/line_slope); //yikes, this won't work when the line is perpendicular
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
