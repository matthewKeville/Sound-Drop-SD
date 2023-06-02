#ifndef SPAWNER_H
#define SPAWNER_H

#include "shader.h"
#include "Ball.h"
#include "Interactable.h"

class Spawner : public Interactable {
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
    Ball* spawn(float currentTime);
    double lastSpawn;
    //Interactable
    void move(float x, float y);
    void position(float x,float y);
    bool IsHovering(float,float);

  private:
    unsigned int sides;
    float radius;
};

#endif
