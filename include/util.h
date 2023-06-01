#ifndef UTIL_H
#define UTIL_H

#include <tuple>

namespace keville::util {


//map line width to piano key numbers
int line_width_to_piano_key(float line_width,int base_key);

//map 1 line width to 1/4 frequency, map 1/2 to 1/2 frequency , 1 to 1 ...
unsigned int line_width_to_frequency_multiple(float line_width) ;

//map piano key numbers to frequencies (unused)
unsigned int piano_key_to_frequency(int key) ;

//please please please refactor these
float line_width_to_frequency_multiple_chromatic(float line_width);
int line_width_to_frequency_multiple_chromatic_exponent(float line_width); 


extern unsigned int BASE_FREQUENCY_RATE;
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
