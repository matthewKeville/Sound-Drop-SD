#ifndef BALL_H
#define BALL_H

#include "shader.h"
#include <glm/glm.hpp>

class Ball {
  public:

    Ball(Shader*,const unsigned int*,const unsigned int*,float,float);
    void draw();

    void setPosition(glm::vec2);
    glm::vec2 getPosition();
    void setVelocity(glm::vec2);
    glm::vec2 getVelocity();
    void print();

  private:
    Shader* shader;
    const unsigned int* vao;
    const unsigned int* vbo;
    glm::mat4 model;
    float radius;
    void updateModelMatrix();
    glm::vec2 center;
    glm::vec2 velocity;
};

#endif
