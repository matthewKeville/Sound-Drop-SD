#include "Spawner.h"
#include <stdexcept>
#include <cmath>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "util.h"

Spawner::Spawner( Shader* shader,const unsigned int* spawnerVao,const unsigned int* spawnerVbo,
                  Shader* digitShader, const unsigned int* digitVao,const unsigned int* digitVbo ,const unsigned int* digitTextures,
                  Shader* ballShader, const unsigned int* ballVao,const unsigned int* ballVbo,
                  float cx, float cy,float baseFrequency,unsigned int scale) {

  this->radius = 0.02f;
  this->center = glm::vec2(cx,cy);

  this->baseFrequency = baseFrequency;
  this->scale = scale;
  this->lastQuantumSpawn = 0;

  this->shader = shader;
  this->digitShader = digitShader;
  this->ballShader = ballShader;

  this->vao = spawnerVao;
  this->vbo = spawnerVbo;
  this->digitVao = digitVao;
  this->digitVbo = digitVbo;
  this->digitTextures = digitTextures;
  this->ballVao = ballVao;
  this->ballVbo = ballVbo;

  updateModelMatrix();

}

//redundancy here for sure
void Spawner::draw() {

  //draw spawner circle

  shader->use();
  int ColorLoc = glGetUniformLocation(shader->ID, "Color"); 
  glUniform3f(ColorLoc,1.f,1.f,1.f); 
  int ModelLoc = glGetUniformLocation(shader->ID, "Model"); 
  glUniformMatrix4fv(ModelLoc, 1, GL_FALSE, glm::value_ptr(this->model));

  glLineWidth(2.0f);
  glBindBuffer(GL_ARRAY_BUFFER,*vbo);
  glBindVertexArray(*vao); 
  glDrawArrays(GL_LINES, 0, 3*keville::util::CIRCLE_SIDES);

  //draw spawner digit

  digitShader->use();

  int digitModelLoc = glGetUniformLocation(digitShader->ID, "Model"); 
  glUniformMatrix4fv(digitModelLoc, 1, GL_FALSE, glm::value_ptr(this->digitModel));

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, this->digitTextures[this->scale]);

  int digitTextureLoc = glGetUniformLocation(digitShader->ID, "Texture0"); 
 glUniform1f(digitTextureLoc,digitTextures[0]);


  glBindBuffer(GL_ARRAY_BUFFER,*this->digitVbo);
  glBindVertexArray(*this->digitVao); 

  //Without this, transparency doesn't work. I don't really know what this is yet
  //https://www.reddit.com/r/opengl/comments/5ups85/struggling_with_png_transparency_via_stb_image/
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glDrawArrays(GL_TRIANGLES, 0, 6);

  //digits



}



/*
 * This approach allows us to synchronize various spawners with little overhead
 */
Ball* Spawner::spawn(float currentTime) {

  float wavelength = (1.f / this->baseFrequency ); //is this a safe cast?
  float quantum = wavelength/this->scale;
  float epsilon = 0.05f;  //50ms  | note : .01 or (10ms) caused noticeable missed quantums at fixed intervals

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

//(wscp) : world coordinates of mouse position
bool Spawner::IsHovering(glm::vec2 wscp) {
  //we "move" the circle that represents a spawner to the origin
  //and the ndc coordintes to see if they are within the circle 
  auto local = wscp - this->center;
  float local_r = sqrt(glm::dot(local,local));
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
  std::cout << " spawner scale set to " << scale << std::endl;
}

unsigned int Spawner::getScale() {
  return this->scale;
}

void Spawner::updateModelMatrix() {
  glm::mat4 baseModel = glm::mat4(1.0f);
  baseModel = glm::translate(baseModel,glm::vec3(center.x,center.y,0));   //translate into place
  this->model = glm::scale(baseModel,this->radius * glm::vec3(1));

  //the spawner scale indicator will be render 30 degrees above the spawner
  //float angle = M_PI/60; //30 degrees
  float angle = 3.14f/60; //30 degrees
  float r = 0.05f;
  float digitScale = 0.040f;
  float aspect = 0.5f;
  this->digitModel = glm::translate(baseModel,glm::vec3(r*cos(angle),r*sin(angle),0.0f));
  this->digitModel = glm::scale(this->digitModel,glm::vec3(digitScale,digitScale,0.0f));
  this->digitModel = glm::scale(this->digitModel,glm::vec3(aspect,1.0f,1.0f));

}

glm::vec2 Spawner::getPosition() {
  return this->center;
}


std::ostream& operator<<(std::ostream& os,Spawner& s) {
  auto pos = s.getPosition();
  return os << "{" << pos.x << ","  << pos.y << "," << s.getScale() << "}";
}


