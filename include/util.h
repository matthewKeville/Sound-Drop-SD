#ifndef UTIL_H
#define UTIL_H

#include <tuple>
#include <functional>

namespace keville::util {

//map piano key numbers to frequencies (unused)
unsigned int piano_key_to_frequency(int key) ;

//new
int line_width_to_semitone_logarithmic_chromatic(float line_width,unsigned int octaves); 
int line_width_to_semitone_linear_chromatic(float line_width,unsigned int octaves); 
unsigned int semitone_adjusted_rate(float base_rate,int semitones);
std::tuple<float,float,float> semitone_color_chromatic(int semitone);
float* generate_regular_polygon_vertices(unsigned int sides,float radius,int& vertex_total);
float* generate_regular_polygon_hull_vertices(unsigned int sides,float radius,int& vertex_total);


                                   
extern std::function<int(float width)> SEMITONE_WIDTH_MAP;                                 
extern std::function<std::tuple<float,float,float>(int semitones)> SEMITONE_COLOR_MAP;                                 
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
