#include "util.h"
#include <cmath>
#include <iostream>
#include <tuple>

//https://stackoverflow.com/questions/8971824/multiple-definition-of-namespace-variable-c-compilation
//namespace variable rules
namespace keville::util {

//map piano key numbers to frequencies (unused)
unsigned int piano_key_to_frequency(int key) {
  int reference_key = 44;
  unsigned int reference_pitch = 440;
  unsigned int pitch = reference_pitch * pow(pow(2,-12),key-reference_key);
  return pitch;
}

//this construction maps an in infinite number of octaves (which we bound by a set number of octaves)
//the span of a note is proportional to the span of the line, as the line gets smaller, more octaves are 
//traversed exponentially. This gives the affect that a doubled line is double pitch.
int line_width_to_semitone_logarithmic_chromatic(float line_width,unsigned int octaves) {
  float line_width_center  = 0.25f;
  float center_offset = -1.0f*pow(1.0f/(line_width_center/2),1.0f/2.0f) + 1.0f;
  //break line width into a reversed logarithm scale
  float log2_line_width = log(line_width) / log(2);
  //float scale = -log2_line_width - 1; //  1/4 should be same pitch
  float scale = -log2_line_width + center_offset; //  1/4 should be same pitch
  //break this point into whole and fractional parts
  float iscale;
  float fpscale = std::modf(scale, &iscale); 
  //find the nearest 12th that fpscale fits into
  float nearest_12th = floor(fpscale*12.0f); 
  //calculate a scalar such that scaling any frequency by this number
  //results in a pitch that exists in an equal temperment chromatic scale
  //(where the base pitch is arbitrary)
  float semitones = iscale*12.0f + nearest_12th;
  std::cout << "raw exp " << semitones << std::endl;
  //clamp the semitone between our octave range
  semitones = std::min(octaves*12.0f,semitones);
  semitones = std::max(-1.0f*octaves*12.0f,semitones);
  return (int) semitones;
}


//semitone_radius : how many semitone in each direction
int line_width_to_semitone_linear_chromatic(float line_width,unsigned int semitone_radius) {
  float line_span  = keville::util::MAX_LINE_WIDTH-keville::util::MIN_LINE_WIDTH;
  float center = keville::util::MIN_LINE_WIDTH + (line_span/2.0f);
  float width_bucket = (line_span) / (2.f * semitone_radius); 
  float center_distance = center - line_width ;
  float fsemi = center_distance / width_bucket;
  int semitones = floor(fsemi);
  return semitones;
}

//semitone_radius : how many semitone in each direction
//return a semitone distance that is within the major scale
int line_width_to_semitone_linear_major(float line_width,unsigned int semitone_radius) {
  int scale[7] = { 0 , 2 , 4 , 5 , 7 , 9 , 11 }; //semitones away for each note in the scale
  int chroma_semitones = line_width_to_semitone_linear_chromatic(line_width,semitone_radius);
  int octaves = chroma_semitones / 7;
  int position = chroma_semitones % 7;
  if ( chroma_semitones < 0 ) {
    position = 7 - (-chroma_semitones % 7);
  }
  return octaves*12 + scale[position];
}

//semitone_radius : how many semitone in each direction
//return a semitone distance that is within the major pentatonic scale
int line_width_to_semitone_linear_major_pentatonic(float line_width,unsigned int semitone_radius) {
  int scale[5] = { 0 , 2 , 4 ,  7 , 9  }; //semitones away for each note in the scale
  int chroma_semitones = line_width_to_semitone_linear_chromatic(line_width,semitone_radius);
  int octaves = chroma_semitones / 5;
  int position = chroma_semitones % 5;
  if ( chroma_semitones < 0 ) {
    position = 5 - (-chroma_semitones % 5);
  }
  return octaves*12 + scale[position];
}


//semitone_radius : how many semitone in each direction
//return a semitone distance that is within the minor scale
int line_width_to_semitone_linear_minor(float line_width,unsigned int semitone_radius) {
  int scale[7] = { 0 , 2 , 3 , 5 , 7 , 8 , 10 }; //semitones away for each note in the scale
  int chroma_semitones = line_width_to_semitone_linear_chromatic(line_width,semitone_radius);
  int octaves = chroma_semitones / 7;
  int position = chroma_semitones % 7;
  if ( chroma_semitones < 0 ) {
    position = 7 - (-chroma_semitones % 7);
  }
  return octaves*12 + scale[position];
}

//semitone_radius : how many semitone in each direction
//return a semitone distance that is within the minor pentatonic scale
int line_width_to_semitone_linear_minor_pentatonic(float line_width,unsigned int semitone_radius) {
  int scale[5] = { 0 ,  3 , 5 , 7 , 10 }; //semitones away for each note in the scale
  int chroma_semitones = line_width_to_semitone_linear_chromatic(line_width,semitone_radius);
  int octaves = chroma_semitones / 5;
  int position = chroma_semitones % 5;
  if ( chroma_semitones < 0 ) {
    position = 5 - (-chroma_semitones % 5);
  }
  return octaves*12 + scale[position];
}

//semitone_radius : how many semitone in each direction
//return a semitone distance that is within the minor pentatonic scale
int line_width_to_semitone_linear_blues(float line_width,unsigned int semitone_radius) {
  int scale[6] = { 0 ,  3 , 5 , 6 , 7 , 10 }; //blues is minor penta but with maj flat 5
  int chroma_semitones = line_width_to_semitone_linear_chromatic(line_width,semitone_radius);
  int octaves = chroma_semitones / 6;
  int position = chroma_semitones % 6;
  if ( chroma_semitones < 0 ) {
    position = 6 - (-chroma_semitones % 6);
  }
  return octaves*12 + scale[position];
}


//return a rate in hz, that is 'semitones' away from base_rate
unsigned int semitone_adjusted_rate(float base_rate,int semitones) {
  float scale_factor = pow(pow(2,1.0f/12.0f),semitones); 
  return ceil(base_rate*scale_factor);
}

std::tuple<float,float,float> semitone_color_chromatic(int semitone) {
  if ( semitone > 0 ) {
    return keville::util::COLOR_WHEEL_12[semitone % 12];
  }
  return keville::util::COLOR_WHEEL_12[-semitone % 12];
}



//create a new array an point vertices towards it
//return the size of the array pointed to
float* generate_regular_polygon_vertices(unsigned int sides,float radius,int& vertex_total) {
  float* vertices = new float[sides*3*3]{};
  //generate vertex data

  float theta = 2*M_PI / sides; //wow, i forgot about radians, sadge
  for ( size_t i = 0; i < sides; i++ ) {

    int j = i * 9;
    float phi0 = i*theta;
    float phif = (i+1)*theta;

    //ball center

    vertices[j]   = 0;
    vertices[j+1] = 0;
    vertices[j+2] = 0;  

    //lower angle vertex
    float vx0 = radius*cos(phi0);
    float vy0 = radius*sin(phi0);

    vertices[j+3]   = vx0;
    vertices[j+4]   = vy0;
    vertices[j+5]   = 0; 
   
    //upper angle vertex 
    float vxf = radius*cos(phif);
    float vyf = radius*sin(phif);

    vertices[j+6]   = vxf;
    vertices[j+7]   = vyf;
    vertices[j+8]   = 0;  
  }
  vertex_total = 3*3*sides;
  return vertices;
}

//no interior vertex (render as lines)
float* generate_regular_polygon_hull_vertices(unsigned int sides,float radius,int& vertex_total) {
  float* vertices = new float[sides*2*3]{};
  //generate vertex data

  float theta = 2*M_PI / sides; //wow, i forgot about radians, sadge
  for ( size_t i = 0; i < sides; i++ ) {

    int j = i * 6;
    float phi0 = i*theta;
    float phif = (i+1)*theta;


    //lower angle vertex
    float vx0 = radius*cos(phi0);
    float vy0 = radius*sin(phi0);

    vertices[j+0]   = vx0;
    vertices[j+1]   = vy0;
    vertices[j+2]   = 0; 
   
    //upper angle vertex 
    float vxf = radius*cos(phif);
    float vyf = radius*sin(phif);

    vertices[j+3]   = vxf;
    vertices[j+4]   = vyf;
    vertices[j+5]   = 0;  

  }
  vertex_total = 3*2*sides;
  return vertices;
}

  const unsigned int CIRCLE_SIDES = 50;
  const float MAX_LINE_WIDTH = 2.0f;
  const float MIN_LINE_WIDTH = 0.1f;
  const std::tuple<float,float,float> COLOR_RED (148.0f/255.0f,0.0f,221.0f/255.0f);
  const std::tuple<float,float,float> COLOR_RED_ORANGE (100.0f/255.0f,32.5f/255.0f,28.6f/255.0f); //12 tone
  const std::tuple<float,float,float> COLOR_ORANGE (1.0f,127.0f/255.0f,0.0f);
  const std::tuple<float,float,float> COLOR_ORANGE_YELLOW (1.0f,0.682f,0.259f);
  const std::tuple<float,float,float> COLOR_YELLOW (1.0f,1.0f,0.0f);
  const std::tuple<float,float,float> COLOR_YELLOW_GREEN (0.604f,0.804f,0.196f);
  const std::tuple<float,float,float> COLOR_GREEN (0.0f,1.0f,0.0f);
  const std::tuple<float,float,float> COLOR_GREEN_BLUE (0.51f,0.596f,0.729f);
  const std::tuple<float,float,float> COLOR_BLUE (0.0f,0.0f,1.0f);
  //pass
  const std::tuple<float,float,float> COLOR_INDIGO(75.0f/255.0f,0.0f,130.0f/255.0f);
  //pass
  const std::tuple<float,float,float> COLOR_VIOLET(148.0f/255.0f,0.0f,211.0f/255.0f);
  const std::tuple<float,float,float> COLOR_VIOLET_RED(0.96f,0.325f,0.58f);

  std::tuple<float,float,float> COLOR_WHEEL_12[12] = {
    COLOR_RED,COLOR_RED_ORANGE,
    COLOR_ORANGE,COLOR_ORANGE_YELLOW,
    COLOR_YELLOW,COLOR_YELLOW_GREEN,
    COLOR_GREEN,COLOR_GREEN_BLUE,
    COLOR_BLUE,COLOR_INDIGO,
    COLOR_VIOLET,COLOR_VIOLET_RED
  };

  std::tuple<float,float,float> COLOR_WHEEL_7[7] = {
    COLOR_RED,
    COLOR_ORANGE,
    COLOR_YELLOW,
    COLOR_GREEN,
    COLOR_BLUE,
    COLOR_VIOLET,
    COLOR_VIOLET_RED
  };

}
