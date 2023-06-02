#include "Line.h"
#include <glad/glad.h>
#include "util.h"
#include <cmath>

Line::Line(Shader* shader,float x0,float y0, float xf, float yf) {
  this->shader = shader;
  //TODO check if vertices are in NDC
  //Assemble vertex data
  vertices = new float[6];
  vertices[0] = x0;
  vertices[1] = y0;
  vertices[2] = 0.0f;
  vertices[3] = xf;
  vertices[4] = yf;
  vertices[5] = 0.0f;
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
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * 3, vertices, GL_STATIC_DRAW);//stores 2 lines

  //assign tone & color
  float line_width = sqrt( pow(yf-y0,2) + pow(xf-x0,2) );

  semitone = keville::util::SEMITONE_WIDTH_MAP(line_width);
  color = keville::util::SEMITONE_COLOR_MAP(semitone);

  //assign color
  std::cout << "line semitone : " << semitone << std::endl;
  std::cout << "line width    : " << line_width << std::endl;
}


void Line::draw() {
  shader->use();
  int ColorLoc = glGetUniformLocation(shader->ID, "Color"); 
  glBindBuffer(GL_ARRAY_BUFFER,vbo);
  glBindVertexArray(vao); 
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 2 * 3, vertices);
  //glUniform3f(ColorLoc,1.0f,0.0f,1.0f);
  glUniform3f(ColorLoc,std::get<0>(color),std::get<1>(color),std::get<2>(color));
  glLineWidth(6.0f);
  glDrawArrays(GL_LINES, 0, 2);
}

void Line::print() {
  char msg[120];
  char msg2[120];
  sprintf(msg,"vertex data\n\tx0:%f\ty0:%f\tz0:%f",vertices[0],vertices[1],vertices[2]);
  sprintf(msg2,"\t\nxf:%f\tyf:%f\tzf:%f",vertices[3],vertices[4],vertices[5]);
  std::cout << msg << std::endl;
  std::cout << msg2 << std::endl;
}
