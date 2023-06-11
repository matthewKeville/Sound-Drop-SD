#ifndef SPAWNER_H
#define SPAWNER_H

#include "shader.h"
#include "Ball.h"
#include "Interactable.h"

#include <glm/glm.hpp>

class Spawner : public Interactable {
  public:
    void draw();
    Spawner(Shader*,Shader*,const unsigned int*,const unsigned int*,const unsigned int*,const unsigned int*,float,float,float,float);
    Ball* spawn(float currentTime/*in s from app start */);
    //Interactable
    void move(float x, float y);
    void position(float x,float y);
    bool IsHovering(float,float);
    void setScale(unsigned int);
    unsigned int getScale();
  private:
    //opengl buffers main circle
    float* vertices; 
    //opengl buffers scale circle
    unsigned int vaoScale;
    unsigned int vboScale;
    float* verticesScale; 
    
    Shader* shader;
    Shader* ballShader;

    //geometric properties
    unsigned int sides;
    float radius;
    glm::vec2 center;
    //spawner characteristics
    float baseFrequency; //spawn frequency in Hz
    unsigned int scale;
    unsigned int lastQuantumSpawn;
    const unsigned int MAX_SCALE = 10;
    float lastTime;
    double lastSpawn;

    const unsigned int* ballVao;
    const unsigned int* ballVbo;
    const unsigned int* vao;
    const unsigned int* vbo;

    glm::mat4 model;
    void updateModelMatrix();

};

#endif
