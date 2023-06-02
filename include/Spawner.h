#ifndef SPAWNER_H
#define SPAWNER_H

#include "shader.h"
#include "Ball.h"

class Spawner {
  public:
    unsigned int vao;
    unsigned int vbo;
    float* vertices; 

    float cx;
    float cy;

    float lastTime;
    float frequency;

    void draw();
    Spawner(Shader*,Shader*,float,float,float);
    Shader* shader;
    Shader* ballShader;
    void move(float x, float y);

    Ball* spawn(float currentTime);
    double lastSpawn;


  private:
    unsigned int sides;
    float radius;
};

#endif
