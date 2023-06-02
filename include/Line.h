#ifndef LINE_H
#define LINE_H

#include "shader.h"
#include "Interactable.h"
#include <tuple>

class Line : public Interactable {
  public:
    unsigned int vao;
    unsigned int vbo;
    float* vertices; 
    void draw();
    Line(Shader*,float,float,float,float);
    Shader* shader;
    void print();
    std::tuple<float,float,float> color;
    int semitone;
    //Interactable
    bool IsHovering(float,float);
    void move(float x,float y);
    void position(float x,float y);

};

#endif
