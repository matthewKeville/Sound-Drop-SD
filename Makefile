all: sound-drop.exe 
sound-drop.exe: build/
	g++ -g -Wall -Wextra  -I./include/ src/glad.c src/sound-drop.cpp src/Line.cpp src/Ball.cpp src/util.cpp -o build/sound-drop.exe -lglfw -lGL
build/ :
	mkdir build
clean : 
	rm -rf build
