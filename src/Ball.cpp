#include "Ball.h"
#include <glad/glad.h>
#include <stdexcept>
#include <cmath>

//the construction code for this class should be lifted to a more general namespace that can create
//geometries. This code is for the creation of vertex data that represent ngons
Ball::Ball(Shader* shader,int sides,float cx, float cy) {
  this->radius = 0.02f;
  this->shader = shader;
  this->sides = sides;
  this->cx = cx;
  this->cy = cy;
  //this->vx = 0;
  this->vx = 0.00001f; // line intersection needs to be fixed 
  this->vy = 0;

  //TODO check if vertices are in NDC
  if ( cx < -1 || cy > 1 ) {
    throw std::invalid_argument(" cx & cy , must be in the range [-1,1]");
  }
 
  this->vertices = new float[sides*3*3]{};
  //generate vertex data

  float theta = 2*M_PI / sides; //wow, i forgot about radians, sadge
  for ( int i = 0; i < sides; i++ ) {

    int j = i * 9;
    float phi0 = i*theta;
    float phif = (i+1)*theta;

    //ball center

    this->vertices[j]   = 0;
    this->vertices[j+1] = 0;
    this->vertices[j+2] = 0;  

    //lower angle vertex
    float vx0 = this->radius*cos(phi0);
    float vy0 = this->radius*sin(phi0);

    this->vertices[j+3]   = vx0;
    this->vertices[j+4]   = vy0;
    this->vertices[j+5]   = 0; 
   
    //upper angle vertex 
    float vxf = this->radius*cos(phif);
    float vyf = this->radius*sin(phif);

    this->vertices[j+6]   = vxf;
    this->vertices[j+7]   = vyf;
    this->vertices[j+8]   = 0;  

  }


  //generate buffers
  glGenVertexArrays(1, &vao);
  glGenBuffers(1,&vbo);
  //assemble vertex array
  glBindBuffer(GL_ARRAY_BUFFER,vbo);
  glBindVertexArray(vao);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0); 
  glBindVertexArray(0);
 
  //initialize vertex buffer
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * 3 * this->sides, vertices, GL_STATIC_DRAW);//stores 2 lines

}

void Ball::draw() {
  shader->use();

  int ColorLoc = glGetUniformLocation(shader->ID, "Color"); 
  glUniform3f(ColorLoc,1.f,1.f,1.f);
  int WorldPositionLoc = glGetUniformLocation(shader->ID, "WorldPosition"); 
  glUniform2f(WorldPositionLoc,this->cx,this->cy);

  glBindBuffer(GL_ARRAY_BUFFER,vbo);
  glBindVertexArray(vao); 
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 3 * 3 * this->sides, vertices);
  glDrawArrays(GL_TRIANGLES, 0, 3*this->sides);
}

void Ball::print() {
  for ( int i = 0; i < this->sides; i++) {
    std::cout << "side= " << i << std::endl;
    for ( int j = 0; j < 3; j++ ) {
      int k = 9*i + (3*j);
    }
  }
}

void Ball::move(float x,float y) {
  this->cx = x;
  this->cy = y;
}
