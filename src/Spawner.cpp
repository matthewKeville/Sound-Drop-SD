#include "Spawner.h"
#include <glad/glad.h>
#include <stdexcept>
#include <cmath>
#include "util.h"

//i think there is a memory leak here

Spawner::Spawner(Shader* shader,Shader* ballShader,float frequency,float cx, float cy) {
  this->radius = 0.02f;
  this->sides = 50;
  this->shader = shader;
  this->ballShader = ballShader;
  this->frequency = frequency;
  this->cx = cx;
  this->cy = cy;

  //TODO check if vertices are in NDC
  if ( cx < -1 || cy > 1 ) {
    throw std::invalid_argument(" cx & cy , must be in the range [-1,1]");
  }
 
  int vertex_total = 0;
  this->vertices = keville::util::generate_regular_polygon_hull_vertices(this->sides,this->radius,vertex_total);

  //generate buffers
  glGenVertexArrays(1, &vao);
  glGenBuffers(1,&vbo);
  //assemble vertex array
  glBindBuffer(GL_ARRAY_BUFFER,vbo);
  glBindVertexArray(vao);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0); 
  glBindVertexArray(0);
 
  //initialize vertex buffer
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * 2 * this->sides, vertices, GL_STATIC_DRAW);

}

void Spawner::draw() {
  shader->use();

  int ColorLoc = glGetUniformLocation(shader->ID, "Color"); 
  glUniform3f(ColorLoc,1.f,1.f,1.f);
  int WorldPositionLoc = glGetUniformLocation(shader->ID, "WorldPosition"); 
  glUniform2f(WorldPositionLoc,this->cx,this->cy);

  glLineWidth(2.0f);
  glBindBuffer(GL_ARRAY_BUFFER,vbo);
  glBindVertexArray(vao); 
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 3 * 2 * this->sides, vertices);
  
  glDrawArrays(GL_LINES, 0, 3*this->sides);
}


void Spawner::move(float x,float y) {
  this->cx = x;
  this->cy = y;
}


Ball* Spawner::spawn(float currentTime) {
  if ( currentTime > lastSpawn + (1/frequency) ) {
    lastSpawn = currentTime;
    return new Ball(this->ballShader,this->sides,this->cx,this->cy);
  }
  return nullptr;
}













