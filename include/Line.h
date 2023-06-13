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
    Line(Shader*,const unsigned int*,const unsigned int*,float,float,float,float,std::function<int(float width)>,std::function<std::tuple<float,float,float>(int semitones)> );
    void draw();
    void calculateToneAndColor(std::function<int(float)>,
        std::function<std::tuple<float,float,float>(int)>);
    std::tuple<glm::vec2,glm::vec2> getPosition();
    void updatePoints(glm::vec2,glm::vec2);
    //Interactable
    void move(float x,float y);
    void position(float x,float y);
    bool IsHovering(glm::vec2 wscp);
  private :
    Shader* shader;
    const unsigned int* vao;
    const unsigned int* vbo;
    glm::mat4 model;
    glm::vec2 pointA;
    glm::vec2 pointB;
    void updateModelMatrix();
};

#endif
