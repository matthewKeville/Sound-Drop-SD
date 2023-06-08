#ifndef LINE_H
#define LINE_H

#include "shader.h"
#include "Interactable.h"
#include <tuple>
#include <functional>

class Line : public Interactable {
  public:
    unsigned int vao;
    unsigned int vbo;
    float* vertices; 
    void draw();
    Line(Shader*,float,float,float,float,std::function<int(float width)>,std::function<std::tuple<float,float,float>(int semitones)> );
    Shader* shader;
    void print();
    std::tuple<float,float,float> color;
    int semitone;
    void calculateToneAndColor(std::function<int(float)>,
        std::function<std::tuple<float,float,float>(int)>);
    //Interactable
    bool IsHovering(float,float);
    void move(float x,float y);
    void position(float x,float y);
    bool isDeleted();
    void markDeleted();
};

#endif
