#ifndef BALL_H
#define BALL_H

#include "shader.h"

class Ball {
  public:
    unsigned int vao;
    unsigned int vbo;
    float* vertices; 

    float cx;
    float cy;

    float vx;
    float vy;

    void draw();
    Ball(Shader*,int,float,float);
    Shader* shader;
    void print();
    void move(float x, float y);
  private:
    unsigned int sides;
    float radius;
};

#endif
