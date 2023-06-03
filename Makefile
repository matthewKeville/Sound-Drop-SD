SOLOUD_HEADERS = /home/galaxy/code/foss/soloud/include
SOLOUD_SO = /home/galaxy/code/foss/soloud/lib/
SOLOUD_A = /home/galaxy/code/foss/soloud/lib/

all: sound-drop.exe 
sound-drop.exe: build/
	g++ -g -Wall -Wextra  -I./include/ -I$(SOLOUD_HEADERS) -L$(SOLOUD_A)  src/glad.c src/sound-drop.cpp src/Line.cpp src/Ball.cpp src/util.cpp src/Spawner.cpp src/Interactable.cpp -o build/sound-drop.exe -lglfw -lGL -lsoloud_static -lasound
build/ :
	mkdir build
clean : 
	rm -rf build
