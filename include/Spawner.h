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
    unsigned int vaoScale;
    unsigned int vboScale;
    float* verticesScale; 
    float cx;
    float cy;
    float lastTime;
    void draw();
    Spawner(Shader*,Shader*,float,float,float,unsigned int);
    Shader* shader;
    Shader* ballShader;
    /* currentTime : time in seconds since application start */
    Ball* spawn(float currentTime);
    double lastSpawn;
    //Interactable
    void move(float x, float y);
    void position(float x,float y);
    bool IsHovering(float,float);
    void setScale(unsigned int);
    unsigned int getScale();

  private:
    unsigned int sides;
    float radius;
    float baseFrequency; //spawn frequency in Hz
    unsigned int scale;
    unsigned int lastQuantumSpawn;
    const unsigned int MAX_SCALE = 10;
};

#endif
