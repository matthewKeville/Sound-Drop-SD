#ifndef UTIL_H
#define UTIL_H

#include <tuple>
#include <functional>

namespace keville::util {

//map piano key numbers to frequencies (unused)
unsigned int piano_key_to_frequency(int key) ;

//line width to semitone mapping
int line_width_to_semitone_logarithmic_chromatic(float line_width,unsigned int octaves); 
int line_width_to_semitone_linear_chromatic(float line_width,unsigned int octaves); 

//semitone_radius : how many semitone in each direction
//return a semitone distance that is within the major scale
int line_width_to_semitone_linear_major(float line_width,unsigned int semitone_radius);

//semitone_radius : how many semitone in each direction
//return a semitone distance that is within the major pentatonic scale
int line_width_to_semitone_linear_major_pentatonic(float line_width,unsigned int semitone_radius);

//semitone to rate maps
unsigned int semitone_adjusted_rate(float base_rate,int semitones);
//semitone to color maps
std::tuple<float,float,float> semitone_color_chromatic(int semitone);
//vertex data generation
float* generate_regular_polygon_vertices(unsigned int sides,float radius,int& vertex_total);
float* generate_regular_polygon_hull_vertices(unsigned int sides,float radius,int& vertex_total);

extern const float MAX_LINE_WIDTH; 
extern const float MIN_LINE_WIDTH; 
extern unsigned int SAMPLE_BASE_RATE; //to be set in main
extern const std::tuple<float,float,float> COLOR_RED;
extern const std::tuple<float,float,float> COLOR_RED_ORANGE;
extern const std::tuple<float,float,float> COLOR_ORANGE;
extern const std::tuple<float,float,float> COLOR_ORANGE_YELLOW;
extern const std::tuple<float,float,float> COLOR_YELLOW;
extern const std::tuple<float,float,float> COLOR_YELLOW_GREEN;
extern const std::tuple<float,float,float> COLOR_GREEN;
extern const std::tuple<float,float,float> COLOR_GREEN_BLUE;
extern const std::tuple<float,float,float> COLOR_BLUE;
extern const std::tuple<float,float,float> COLOR_INDIGO;
extern const std::tuple<float,float,float> COLOR_VIOLET;
extern const std::tuple<float,float,float> COLOR_VIOLET_RED;
extern std::tuple<float,float,float> COLOR_WHEEL_12[12];


}

#endif
