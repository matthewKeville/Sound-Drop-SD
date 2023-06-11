#include "Line.h"
#include <glad/glad.h>
#include "util.h"
#include <cmath>
#include <functional>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Line::Line(Shader* shader,const unsigned int* vao, const unsigned int* vbo, float x0, float y0, float xf, float yf,
    std::function<int(float width)> widthSemitoneMap,
    std::function<std::tuple<float,float,float>(int)> semitoneColorMap) {

  this->shader = shader;
  this->vao = vao;
  this->vbo = vbo;

  this->pointA = glm::vec2(x0,y0);
  this->pointB = glm::vec2(xf,yf);

  updateModelMatrix();
  calculateToneAndColor(widthSemitoneMap,semitoneColorMap);
}


void Line::draw() {

  shader->use();

  int ColorLoc = glGetUniformLocation(shader->ID, "Color"); 
  glUniform3f(ColorLoc,std::get<0>(color),std::get<1>(color),std::get<2>(color));

  int ModelLoc = glGetUniformLocation(shader->ID, "Model"); 
  glUniformMatrix4fv(ModelLoc, 1, GL_FALSE, glm::value_ptr(this->model));

  glBindBuffer(GL_ARRAY_BUFFER,*this->vbo);
  glBindVertexArray(*this->vao); 

  glLineWidth(6.0f);
  glDrawArrays(GL_LINES, 0, 2);
}


bool Line::IsHovering(float ndcx,float ndcy) {

    /*
     * Translate our Line so that the leftmost coordinate coincides with the origin
     * Perform an orthographic projection onto the line from our point.
     * Use this to determine being bound by the ends of our segments.
     * Use the projection distance to calculate what is considered "near"
     */
  
    //separete our line into ordered components
    float xl = pointA.x;
    float yl = pointA.y;
    float xr = pointB.x;
    float yr = pointB.y;
    if ( pointB.x < pointA.x ) {
      xl = pointB.x;
      yl = pointB.y;
      xr = pointA.x;
      yr = pointA.y;
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
  pointA += glm::vec2(x,y);
  pointB += glm::vec2(x,y);
  updateModelMatrix();
}

void Line::position(float x,float y) {
  glm::vec2 diff2 = pointB - pointA;
  pointA = glm::vec2(x,y);
  pointB = pointA + diff2;
  updateModelMatrix();
}

void Line::updatePoints(glm::vec2 newPointA,glm::vec2 newPointB) {
  this->pointA = newPointA;
  this->pointB = newPointB;
  updateModelMatrix();
}

std::tuple<glm::vec2,glm::vec2> Line::getPosition() {
  return {
    pointA, pointB
  };
}

void Line::updateModelMatrix() {
  //derive a model matrix based on the points of our line segment
  glm::vec2 diff2 = this->pointB - this->pointA;
  glm::vec3 diff = glm::vec3(diff2.x,diff2.y,0);

  float magnitude = sqrt(glm::dot(diff,diff));
  float theta = atan2(diff.y,diff.x);

  this->model = glm::mat4(1.0f);
  this->model = glm::translate(this->model,glm::vec3(pointA.x,pointA.y,0));   //translate into place
  this->model = glm::scale(this->model,magnitude * glm::vec3(1));
  this->model = glm::rotate(this->model, theta, glm::vec3(0,0,1)); //we rotate by the angle of the diff vector
}

void Line::calculateToneAndColor(
    std::function<int(float width)> widthSemitoneMap,
    std::function<std::tuple<float,float,float>(int)> semitoneColorMap) {

  float line_width = sqrt( pow(pointB.y-pointA.y,2) + pow(pointB.x-pointA.x,2) );
  this->semitone = widthSemitoneMap(line_width);
  this->color = semitoneColorMap(semitone);

}


