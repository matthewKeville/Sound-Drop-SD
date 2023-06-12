#include "Spawner.h"
#include <stdexcept>
#include <cmath>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "util.h"

Spawner::Spawner(Shader* shader,Shader* ballShader,const unsigned int* spawnerVao,const unsigned int* spawnerVbo,const unsigned int* ballVao,const unsigned int* ballVbo,float cx, float cy,float
    baseFrequency,float scale) {


  this->radius = 0.02f;
  this->center = glm::vec2(cx,cy);

  this->baseFrequency = baseFrequency;
  this->scale = scale;
  this->lastQuantumSpawn = 0;

  this->ballShader = ballShader;
  this->shader = shader;
  this->vao = spawnerVao;
  this->vbo = spawnerVbo;
  this->ballVao = ballVao;
  this->ballVbo = ballVbo;

  updateModelMatrix();

  //////////////////////////////////////
  //Scale Indicator Rendering Data
  //////////////////////////////////////
  
  //this refactor will be more involved,
  //one approach would be instancing
  //another would be to have n buffers for n scale states
  //and the vbo* could swap when the scale swaps
 
 /* 
  this->verticesScale = new float[3*this->MAX_SCALE]{}; //we allocate enough space so if we change scale
                                                  //we can accomodate the rendering of it's indicator
  float phi = M_PI/8.0; 
  float rdis = this->radius*2.0f;
  for ( size_t i = 0; i < MAX_SCALE; i++ ) {
    float theta = phi*i;
    float px = rdis*cos(theta);
    float py = rdis*sin(theta);
    this->verticesScale[i*3] = px;
    this->verticesScale[(i*3)+1] = py;
    this->verticesScale[(i*3)+2] = 0;
  }

  //generate buffers
  glGenVertexArrays(1, &vaoScale);
  glGenBuffers(1,&vboScale);
  //assemble vertex array
  glBindBuffer(GL_ARRAY_BUFFER,vboScale);
  glBindVertexArray(vaoScale);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0); 
  glBindVertexArray(0);
 
  //initialize vertex buffer
  glBindBuffer(GL_ARRAY_BUFFER, vboScale);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * MAX_SCALE, verticesScale, GL_STATIC_DRAW);
  */

}

//redundancy here for sure
void Spawner::draw() {

  shader->use();

  int ColorLoc = glGetUniformLocation(shader->ID, "Color"); 
  glUniform3f(ColorLoc,1.f,1.f,1.f);
  int ModelLoc = glGetUniformLocation(shader->ID, "Model"); 
  glUniformMatrix4fv(ModelLoc, 1, GL_FALSE, glm::value_ptr(this->model));

  glLineWidth(2.0f);
  glBindBuffer(GL_ARRAY_BUFFER,*vbo);
  glBindVertexArray(*vao); 
  glDrawArrays(GL_LINES, 0, 3*keville::util::CIRCLE_SIDES);

  //draw scale indicators
  /*
  shader->use();
  ColorLoc = glGetUniformLocation(shader->ID, "Color"); 
  glUniform3f(ColorLoc,1.f,1.f,1.f);
  WorldPositionLoc = glGetUniformLocation(shader->ID, "WorldPosition"); 
  glUniform2f(WorldPositionLoc,this->center.x,this->center.y);

  glPointSize(4.0f);
  glBindBuffer(GL_ARRAY_BUFFER,vboScale);
  glBindVertexArray(vaoScale); 
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 3 * this->scale , verticesScale);
  glDrawArrays(GL_POINTS, 0, this->scale);
  */

}



/*
 * This approach allows us to synchronize various spawners with little overhead
 */
Ball* Spawner::spawn(float currentTime) {

  float wavelength = (1.f / this->baseFrequency ); //is this a safe cast?
  float quantum = wavelength/this->scale;
  float epsilon = 0.01f;  //10ms

  float now = currentTime / quantum;
  float leftQuantum = floor(now);
  float rightQuantum = ceil(now);

  //are we within the left quantum and it isn't the last quantum?
  if ( leftQuantum != lastQuantumSpawn && fabs( leftQuantum - now ) < epsilon ) {
    lastQuantumSpawn = leftQuantum;
    return new Ball(this->ballShader,this->ballVao,this->ballVbo,this->center.x,this->center.y);
  }
  if ( rightQuantum != lastQuantumSpawn && fabs( rightQuantum - now ) < epsilon ) {
    lastQuantumSpawn = rightQuantum;
    return new Ball(this->ballShader,this->ballVao,this->ballVbo,this->center.x,this->center.y);
  }

  return nullptr;

}

bool Spawner::IsHovering(float ndcx,float ndcy) {
  //we "move" the circle that represents a spawner to the origin
  //and the ndc coordintes to see if they are within the circle 
  float local_x = ndcx - this->center.x;
  float local_y = ndcy - this->center.y;
  float local_r = sqrt(pow(local_x,2) + pow(local_y,2));
  float tolerance = 1.2f; //enlarge the detection radius slightly more than the actualy shape
  return ( local_r < (this->radius*tolerance));
}

void Spawner::move(float x,float y) {
  this->center.x += x;
  this->center.y += y;
  updateModelMatrix();
}

void Spawner::position(float x,float y) {
  this->center.x = x;
  this->center.y = y;
  updateModelMatrix();
}

void Spawner::setScale(unsigned int scale) {
  this->scale = scale;
}

unsigned int Spawner::getScale() {
  return this->scale;
}

void Spawner::updateModelMatrix() {
  this->model = glm::mat4(1.0f);
  this->model = glm::translate(this->model,glm::vec3(center.x,center.y,0));   //translate into place
  this->model = glm::scale(this->model,this->radius * glm::vec3(1));

}




