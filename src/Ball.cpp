#include "Ball.h"
#include <glad/glad.h>
#include <stdexcept>
#include <cmath>
#include "util.h"

//i think there is a memory leak here

//the construction code for this class should be lifted to a more general namespace that can create
//geometries. This code is for the creation of vertex data that represent ngons
Ball::Ball(Shader* shader,int sides,float cx, float cy) {
  this->radius = 0.02f;
  this->shader = shader;
  this->sides = sides;
  this->cx = cx;
  this->cy = cy;
  this->vx = 0;
  //this->vx = 0.00001f; // line intersection needs to be fixed 
  this->vy = 0;

  //TODO check if vertices are in NDC
  if ( cx < -1 || cy > 1 ) {
    throw std::invalid_argument(" cx & cy , must be in the range [-1,1]");
  }
 
  int vertex_total = 0;
  this->vertices = keville::util::generate_regular_polygon_vertices(this->sides,this->radius,vertex_total);


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
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * 3 * this->sides, vertices, GL_STATIC_DRAW);//stores 2 lines

}

void Ball::draw() {
  shader->use();

  int ColorLoc = glGetUniformLocation(shader->ID, "Color"); 
  glUniform3f(ColorLoc,1.f,1.f,1.f);
  int WorldPositionLoc = glGetUniformLocation(shader->ID, "WorldPosition"); 
  glUniform2f(WorldPositionLoc,this->cx,this->cy);

  glBindBuffer(GL_ARRAY_BUFFER,vbo);
  glBindVertexArray(vao); 
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 3 * 3 * this->sides, vertices);
  glDrawArrays(GL_TRIANGLES, 0, 3*this->sides);
}

void Ball::print() {
}

void Ball::move(float x,float y) {
  this->cx = x;
  this->cy = y;
}
