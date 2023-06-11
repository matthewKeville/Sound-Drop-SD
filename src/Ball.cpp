#include "Ball.h"
#include <glad/glad.h>
#include <stdexcept>
#include <cmath>
#include "util.h"

Ball::Ball(Shader* shader,int sides,float cx, float cy) {
  this->sides = sides;
  this->radius = 0.02f;
  this->center = glm::vec2(cx,cy);
  this->velocity = glm::vec2(0,0);
 
  this->shader = shader;

  int vertex_total = 0;
  /* we don't really need a new set of vertices for each Ball */
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
  glUniform2f(WorldPositionLoc,this->center.x,this->center.y);

  glBindBuffer(GL_ARRAY_BUFFER,vbo);
  glBindVertexArray(vao); 
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 3 * 3 * this->sides, vertices);
  glDrawArrays(GL_TRIANGLES, 0, 3*this->sides);
}

void Ball::print() {
}

void Ball::move(float x,float y) {
  this->center.x = x;
  this->center.y = y;
}
