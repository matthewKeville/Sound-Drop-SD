#include "Line.h"
#include <glad/glad.h>
#include "util.h"
#include <cmath>
#include <functional>

Line::Line(Shader* shader,float x0,float y0, float xf, float yf,
    std::function<int(float width)> widthSemitoneMap,
    std::function<std::tuple<float,float,float>(int)> semitoneColorMap) {

  this->shader = shader;
  this->vertices = new float[6];
  this->vertices[0] = x0;
  this->vertices[1] = y0;
  this->vertices[2] = 0.0f;
  this->vertices[3] = xf;
  this->vertices[4] = yf;
  this->vertices[5] = 0.0f;


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
  calculateToneAndColor(widthSemitoneMap,semitoneColorMap);

}


void Line::draw() {
  shader->use();
  int ColorLoc = glGetUniformLocation(shader->ID, "Color"); 
  glBindBuffer(GL_ARRAY_BUFFER,vbo);
  glBindVertexArray(vao); 
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 2 * 3, vertices);
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

bool Line::IsHovering(float ndcx,float ndcy) {

    /*
     * Translate our Line so that the leftmost coordinate coincides with the origin
     * Perform an orthographic projection onto the line from our point.
     * Use this to determine being bound by the ends of our segments.
     * Use the projection distance to calculate what is considered "near"
     */
  
    //separete our line into ordered components
    float xl;
    float yl;
    float xr;
    float yr;
  
    xl = vertices[0]; yl = vertices[1];
    xr = vertices[3]; yr = vertices[4];
    if (vertices[3] < vertices[0]){
      xr = vertices[0]; yr = vertices[1];
      xl = vertices[3]; yl = vertices[4];
    }

    //find a translation vector to make the (xl,yl) coordinate coincident with the origin
    float tx = -xl;
    float ty = -yl;

    //translate our ndc coordinates
    float nx = ndcx + tx;
    float ny = ndcy + ty;

    float m_line =  ( yr - yl ) / ( xr - xl );
    float m_perp = 1.0f/(-m_line);
    //find the y offset for the linear equation representing
    //the line perpendicular to "this" running through point (nx,ny)
    float y_off = ny - (nx*m_perp);

    //what is the intersection of these two lines?
    float xsol = (y_off) / ( m_line - m_perp );
    float ysol = xsol * m_line;

    float segment_length = sqrt( pow(xr-xl,2) + pow(yr-yl,2) );

    //find the midpoint of the translated line
    float mpx =  (xr - xl)/2;
    float mpy =  (yr - yl)/2;

    float mid_point_distance = sqrt(pow(mpx-xsol,2) + pow(mpy-ysol,2));
    bool withinLineSegments = mid_point_distance < segment_length/2.0f;

    float epsilon = 0.01f; //how far in perpendicular distance the point can be
    float projection_distance = sqrt( pow(nx - xsol,2) + pow(ny - ysol,2));
    bool withinEpsilon = projection_distance < epsilon;

    return withinLineSegments && withinEpsilon;

}


void Line::move(float x,float y) {
  vertices[0] += x;
  vertices[1] += y;
  vertices[3] += x;
  vertices[4] += y;
}

void Line::position(float x,float y) {
  float dx = vertices[3] - vertices[0];
  float dy = vertices[4] - vertices[1];
  vertices[0] = x;
  vertices[1] = y;
  vertices[3] = x + dx;
  vertices[4] = y + dy;
}

std::tuple<glm::vec2,glm::vec2> Line::getPosition() {
  return { 
    glm::vec2(vertices[0],vertices[1]) , 
    glm::vec2(vertices[3],vertices[4]) 
  };
}

void Line::calculateToneAndColor(
    std::function<int(float width)> widthSemitoneMap,
    std::function<std::tuple<float,float,float>(int)> semitoneColorMap) {
    
  //assign tone & color
  float x0 = vertices[0];
  float y0 = vertices[1];
  float xf = vertices[3];
  float yf = vertices[4];

  float line_width = sqrt( pow(yf-y0,2) + pow(xf-x0,2) );
  this->semitone = widthSemitoneMap(line_width);
  this->color = semitoneColorMap(semitone);

}


