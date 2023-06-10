#ifndef BALL_H
#define BALL_H

#include "shader.h"
#include <glm/glm.hpp>

class Ball {
  public:
    glm::vec2 center;
    glm::vec2 velocity;

    void draw();
    Ball(Shader*,int,float,float);
    Shader* shader;
    void print();
    void move(float x, float y);

  private:
    unsigned int vao;
    unsigned int vbo;
    float* vertices; 

    unsigned int sides;
    float radius;
};

#endif
