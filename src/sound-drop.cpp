#include <iostream>
#include <cmath>
#include <vector>
#include <thread>
#include <tuple> 
#include <functional>
#include <algorithm> //std::copy
//#include <chrono> //sleep thread
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

const int MAX_LINES = 50;
const int MAX_BALLS = 300; 

//GLFW
int windowWidth;                
int windowHeight;
GLFWwindow* window;
glm::vec2 mouse; //glfw window coordinates : (0,windowWidth) x (0,windowHeight)

//viewport 
const float MAX_VIEWPORT_SCALE = 3.0f;
const float MIN_VIEWPORT_SCALE = 1.0f;
float viewportScale = MIN_VIEWPORT_SCALE;
glm::vec2 viewportCenter = glm::vec2(0,0);
glm::vec4 clearColor{0.0f,0.0f,0.0f,0.0f};
glm::mat4 projection = glm::ortho(-viewportScale, viewportScale, -viewportScale, viewportScale, -10.f, 10.0f); //left,right ,bot top , near far
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
float* ballVertices; //note float* instead of [], ball resolution can change at runtime
                     
//double ballGravity =  -0.0002f;
double ballGravity =  -0.0002f;
double collisionRestitution = 0.95f;
std::vector<Ball*> balls;
                                                   
//spawners
unsigned int spawnerVao;
unsigned int spawnerVbo;
float* spawnerVertices; //note float* instead of [], ball resolution can change at runtime

double DEFAULT_SPAWN_X = -0.5f;
double DEFAULT_SPAWN_Y = 0.5f;
float DEFAULT_BASE_SPAWN_FREQUENCY = (1.0f/3.0f);
float DEFAULT_SPAWN_SCALE = 1.0f;
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

//saving
const unsigned int NUM_SAVE_SLOTS = 3;
int selectedSaveSlot = 0;
SaveState saveSlots[NUM_SAVE_SLOTS];

StateStack stateStack;


//sim states
bool lineDrawing = false;
bool selected = false;
bool pausePhysics = false;
bool muteAudio = false;
                                                              
//key states
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
void updateView();
void updateProjection();

void record();
void undo();
void redo();

//application utilities
float toDegrees(float radians);
glm::vec2 mouseToNDC(glm::vec2 mouse); //NDC [-1,1] x [1,1]
glm::vec2 ndcToWorldCoordinates(glm::vec2 ndc); 
float viewportCenterRange();


int main() {

  //initialize soloud
  soloud.init();
  soloud.setGlobalVolume(GLOBAL_DEFAULT_VOLUME);
  soloud.setMaxActiveVoiceCount(MAX_VOICE_COUNT);
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
  ImGui_ImplGlfw_InitForOpenGL(window, true);               // configure backends
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
  glClearColor(clearColor.x,clearColor.y,clearColor.z,clearColor.w);


  //load digit textures
  stbi_set_flip_vertically_on_load(true);
  glGenTextures(10,digitTextures);
  for ( int i = 0; i < 10; i++ ) {
    int width, height, nChannels;
    std::string path = "res/textures/digits/digit" + std::to_string(i) + ".png";
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

  digitShader = new Shader("shaders/digit.vs","shaders/digit.fs");
  lineShader = new Shader("shaders/line.vs","shaders/line.fs");
  ballShader = new Shader("shaders/ball.vs","shaders/line.fs");

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

  record();

  while(!glfwWindowShouldClose(window))
  {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

    //start the dear ImGUI frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ///////////////////////////////
    //INPUT
    ///////////////////////////////
   
    bool interactingBeforeInputs = (interactable != nullptr);  /* for tracking interaction end */

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
    //SaveState Interaction complete (delete , move )
    ///////////////////////////////
    
    /* 
     * This is a rather inelegant solution for how interactions are structured.
     * The feature criteria requires new Lines, new Spawners, and relocation and deletion
     * of these entities to be recorded. As a bonus, we get Spawner scale changes
     * for free. I don't love this approach, or rather I don't love how
     * event processing is currently structured.
     */
    if ( interactable == nullptr && interactingBeforeInputs ) {
      record(); 
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
    //const char* scaleNames[scaleData.size()] = {} ;
    const char* scaleNames[scaleData.size()];
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

    // View Port

    bool scaleChanged = false;
    if ( viewportScaleSlider != viewportScale ) {

      scaleChanged = true;

      viewportScale = std::max(MIN_VIEWPORT_SCALE,viewportScaleSlider);
      viewportScale = std::min(MAX_VIEWPORT_SCALE,viewportScaleSlider);

      //update projection
      projection = glm::ortho(-viewportScale, viewportScale, -viewportScale, viewportScale, -10.f, 10.0f); //left,right ,bot top , near far

      updateProjection();

    }

    if ( scaleChanged || viewportCenterSlider != viewportCenter ) {

      //scale changes require checking of the existing view port location
      viewportCenterSlider.x = std::max(-viewportCenterRange(),viewportCenterSlider.x);
      viewportCenterSlider.x = std::min(viewportCenterRange(),viewportCenterSlider.x);
      viewportCenterSlider.y = std::max(-viewportCenterRange(),viewportCenterSlider.y);
      viewportCenterSlider.y = std::min(viewportCenterRange(),viewportCenterSlider.y);

      //update matrix
      viewportCenter = viewportCenterSlider;
      view = glm::translate(glm::mat4(1),glm::vec3(viewportCenter.x,viewportCenter.y,0));
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
      std::cout << " resetting viewport " << std::endl;
      viewportCenter.x = 0;
      viewportCenter.y = 0;
      viewportScale = 1.0f;

      //update projection
      projection = glm::ortho(-viewportScale, viewportScale, -viewportScale, viewportScale, -10.f, 10.0f); //left,right ,bot top , near far
      updateProjection();
      updateView();
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
        auto wscp = ndcToWorldCoordinates(mouseToNDC(mouse));


        spawners.push_back(new Spawner( /*spawner shader is ball shader*/ballShader, &spawnerVao, &spawnerVbo,
                                        digitShader, &digitVao, &digitVbo, digitTextures,
                                        ballShader, &ballVao,&ballVbo,
                                        wscp.x,wscp.y,
                                        DEFAULT_BASE_SPAWN_FREQUENCY,DEFAULT_SPAWN_SCALE));
        record();

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
    std::cout << "Resetting State Stack" << std::endl;
    stateStack.Reset();

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

      ////////////////////////////////
      //interactable selection 
      ////////////////////////////////
      } else if ( !selected && hovered!=nullptr ) {

        selected = true;
        interactable = hovered;
        interactableCenterDisplacement = ndcToWorldCoordinates(mouseToNDC(mouse)) - interactable->getPosition();

      } else if ( selected ) {

        selected = false;
        interactable = nullptr;
      }
    }

    ///////////
    //undo redo
    ///////////
    
    if (button == GLFW_MOUSE_BUTTON_4 && action == GLFW_PRESS) {
      //std::cout << " undo " << std::endl;
      undo();
    }

    if (button == GLFW_MOUSE_BUTTON_5 && action == GLFW_PRESS) {
      //std::cout << " redo " << std::endl;
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

  //constrain the line between our bounds
  
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
