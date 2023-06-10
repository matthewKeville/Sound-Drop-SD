#ifndef LINE_H
#define LINE_H

#include "shader.h"
#include "Interactable.h"
#include <tuple>
#include <functional>
#include <glm/glm.hpp>

class Line : public Interactable {
  public:
    int semitone;
    std::tuple<float,float,float> color;
    Line(Shader*,float,float,float,float,std::function<int(float width)>,std::function<std::tuple<float,float,float>(int semitones)> );
    void draw();
    void print();
    void calculateToneAndColor(std::function<int(float)>,
        std::function<std::tuple<float,float,float>(int)>);
    std::tuple<glm::vec2,glm::vec2> getPosition();
    //Interactable
    void move(float x,float y);
    bool IsHovering(float,float);
    void position(float x,float y);
    bool isDeleted();
    void markDeleted();
  private :
    Shader* shader;
    unsigned int vao;
    unsigned int vbo;
    float* vertices; 
};

#endif
