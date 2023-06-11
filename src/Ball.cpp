#include "Ball.h"
#include <glad/glad.h>
#include <stdexcept>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "util.h"

Ball::Ball(Shader* shader,const unsigned int* vao,const unsigned int* vbo,const int* sides,float cx, float cy) {
  this->radius = 0.02f;
  this->center = glm::vec2(cx,cy);
  this->velocity = glm::vec2(0,0);
 
  this->shader = shader;
  this->vao = vao;
  this->vbo = vbo;
  this->sides = sides;

  updateModelMatrix();
}

void Ball::draw() {
  updateModelMatrix(); //this doesn't belong here, it's here because update_balls
                       //accesses ball->center directly, so updateModelMatrix isn't called
                       //future me needs to refactor so center is encapsulated

  shader->use();

  int ColorLoc = glGetUniformLocation(shader->ID, "Color"); 
  glUniform3f(ColorLoc,1.f,1.f,1.f);

  int ModelLoc = glGetUniformLocation(shader->ID, "Model"); 
  glUniformMatrix4fv(ModelLoc, 1, GL_FALSE, glm::value_ptr(this->model));

  glBindBuffer(GL_ARRAY_BUFFER,*this->vbo);
  glBindVertexArray(*this->vao); 
  glDrawArrays(GL_TRIANGLES, 0, 3*(*this->sides));
}

void Ball::print() {
}

void Ball::move(float x,float y) {
  this->center.x = x;
  this->center.y = y;
  updateModelMatrix();
}

void Ball::updateModelMatrix() {
  this->model = glm::mat4(1.0f);
  this->model = glm::translate(this->model,glm::vec3(center.x,center.y,0));   //translate into place
  this->model = glm::scale(this->model,this->radius * glm::vec3(1));

}
