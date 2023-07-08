#ifndef SPAWNER_H
#define SPAWNER_H

#include "shader.h"
#include "Ball.h"
#include "Interactable.h"
#include <glm/glm.hpp>
#include <ostream>

class Spawner : public Interactable {
  public:
    void draw();
    //spawner digit ball
    Spawner(Shader*,const unsigned int*,const unsigned int*,Shader*,const unsigned int*,const unsigned int*,const unsigned int*,Shader*,const unsigned int*,const unsigned int*,float,float,float,unsigned int);
    Ball* spawn(float currentTime/*in s from app start */);
    void setScale(unsigned int);
    unsigned int getScale();
    //Interactable
    void move(float x, float y);
    void position(float x,float y);
    bool IsHovering(glm::vec2 wscp);
    glm::vec2 getPosition();
  private:
    
    Shader* shader;
    Shader* digitShader;
    Shader* ballShader;

    float radius;
    glm::vec2 center;
    //spawner characteristics
    float baseFrequency; //spawn frequency in Hz
    unsigned int scale;
    unsigned int lastQuantumSpawn;

    const unsigned int* ballVao;
    const unsigned int* ballVbo;

    const unsigned int* digitVao;
    const unsigned int* digitVbo;
    const unsigned int* digitTextures;

    const unsigned int* vao;
    const unsigned int* vbo;

    glm::mat4 model;
    glm::mat4 digitModel;
    void updateModelMatrix();

};

std::ostream& operator<<(std::ostream& os,Spawner& l);

#endif
