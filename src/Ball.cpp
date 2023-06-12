#include "Ball.h"
#include <glad/glad.h>
#include <stdexcept>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "util.h"

Ball::Ball(Shader* shader,const unsigned int* vao,const unsigned int* vbo,float cx, float cy) {
  this->radius = 0.02f;
  this->center = glm::vec2(cx,cy);
  this->velocity = glm::vec2(0,0);
 
  this->shader = shader;
  this->vao = vao;
  this->vbo = vbo;
  updateModelMatrix();
}

void Ball::draw() {
  shader->use();

  int ColorLoc = glGetUniformLocation(shader->ID, "Color"); 
  glUniform3f(ColorLoc,1.f,1.f,1.f);

  int ModelLoc = glGetUniformLocation(shader->ID, "Model"); 
  glUniformMatrix4fv(ModelLoc, 1, GL_FALSE, glm::value_ptr(this->model));

  glBindBuffer(GL_ARRAY_BUFFER,*this->vbo);
  glBindVertexArray(*this->vao); 
  glDrawArrays(GL_TRIANGLES, 0, 3*(keville::util::CIRCLE_SIDES));
}

void Ball::updateModelMatrix() {
  this->model = glm::mat4(1.0f);
  this->model = glm::translate(this->model,glm::vec3(center.x,center.y,0));   //translate into place
  this->model = glm::scale(this->model,this->radius * glm::vec3(1));

}

void Ball::setPosition(glm::vec2 pos) {
  this->center = pos;
  updateModelMatrix();
}

glm::vec2 Ball::getPosition() {
  return this->center;
}

void Ball::setVelocity(glm::vec2 pos) {
  this->velocity = pos;
}

glm::vec2 Ball::getVelocity() {
  return this->velocity;
}

void Ball::print() {
}
