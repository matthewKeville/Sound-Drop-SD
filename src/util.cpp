#include "util.h"
#include <cmath>
#include <iostream>
#include <tuple>

//https://stackoverflow.com/questions/8971824/multiple-definition-of-namespace-variable-c-compilation
//namespace variable rules
//
namespace keville::util {

//map line widths to piano key numbers
int line_width_to_piano_key(float line_width,int base_key) {
  float log2_line_width = log(line_width) / log(2);
  float scale = pow(2,-log2_line_width - 2);
  int key = base_key + ceil(12.0f*scale);
  return key;
}

//map 1 line width to 1/4 frequency, map 1/2 to 1/2 frequency , 1 to 1 ...
unsigned int line_width_to_frequency_multiple(float line_width) {
  float base_rate = 32006;
  float log2_line_width = log(line_width) / log(2);
  float scale = -log2_line_width - 2;
  unsigned int rate = ceil(scale * base_rate);
  return rate;
}

//map piano key numbers to frequencies (unused)
unsigned int piano_key_to_frequency(int key) {
  int reference_key = 44;
  unsigned int reference_pitch = 440;
  unsigned int pitch = reference_pitch * pow(pow(2,-12),key-reference_key);
  return pitch;
}

/*
float line_width_to_frequency_multiple_chromatic(float line_width) {
  float octave_radius = 2;
  //float line_width_center  = 0.5f;
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
  float exponent = iscale*12.0f + nearest_12th;
  std::cout << "raw exp " << exponent << std::endl;
  //clamp the exponent so the greatest scaling is 8x
  exponent = std::min(octave_radius*12.0f,exponent);
  exponent = std::max(-1.0f*octave_radius*12.0f,exponent);
  float final_scale = pow(pow(2,1.0f/12.0f),exponent); //note if i do 1/12 it's fucked because of templating
  std::cout << "exp " << exponent << std::endl;
  return final_scale;
}
*/


//this is fucked (& line_width_to_frequency_multiple_chromatic) refactor urgent
int line_width_to_frequency_multiple_chromatic_exponent(float line_width) {
  float octave_radius = 2;
  //float line_width_center  = 0.5f;
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
  float exponent = iscale*12.0f + nearest_12th;
  std::cout << "raw exp " << exponent << std::endl;
  //clamp the exponent so the greatest scaling is 8x
  exponent = std::min(octave_radius*12.0f,exponent);
  exponent = std::max(-1.0f*octave_radius*12.0f,exponent);
  return (int) exponent;
}

float line_width_to_frequency_multiple_chromatic(float line_width) {
  int exponent = line_width_to_frequency_multiple_chromatic_exponent(line_width);
  float final_scale = pow(pow(2,1.0f/12.0f),exponent); //note if i do 1/12 it's fucked because of templating
  std::cout << "exp " << exponent << std::endl;
  return final_scale;
}

  unsigned int BASE_FREQUENCY_RATE = 32006;
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

}
