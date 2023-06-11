#ifndef BALL_H
#define BALL_H

#include "shader.h"
#include <glm/glm.hpp>

class Ball {
  public:
    glm::vec2 center;
    glm::vec2 velocity;

    void draw();
    Ball(Shader*,const unsigned int*,const unsigned int*,const int*,float,float);
    void print();
    void move(float x, float y);

  private:
    Shader* shader;
    const unsigned int* vao;
    const unsigned int* vbo;
    glm::mat4 model;
    const int* sides;
    float radius;
    void updateModelMatrix();
};

#endif
