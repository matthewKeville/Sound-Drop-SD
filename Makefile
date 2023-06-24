SOLOUD_HEADERS = /home/galaxy/code/foss/soloud/include
SOLOUD_SO = /home/galaxy/code/foss/soloud/lib/
SOLOUD_A = /home/galaxy/code/foss/soloud/lib/

IMGUI_DIR = imgui
IMGUI_SOURCES =  $(IMGUI_DIR)/imgui.cpp  $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp 
IMGUI_SOURCES += $(IMGUI_DIR)/imgui_widgets.cpp
IMGUI_SOURCES += $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp

COMPILER=clang++
#COMPILER=g++

DEV_BUILD_FLAGS=-DDEV_BUILD

STD=c++17

default: build/sound-drop.exe 

build/sound-drop.exe:
	mkdir -p ./build
	$(COMPILER) -std=$(STD) $(DEV_BUILD_FLAGS) -g -Wall -Wextra  -I./include/ -I$(SOLOUD_HEADERS) -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -L$(SOLOUD_A)  src/glad.c src/stb_image.cpp src/sound-drop.cpp src/Line.cpp src/Ball.cpp src/util.cpp src/Spawner.cpp src/Interactable.cpp src/SaveState.cpp src/StateStack.cpp $(IMGUI_SOURCES) -o build/sound-drop.exe -lglfw -lGL -lsoloud_static -lasound


clean : 
	rm -rf build
	rm -rf release


install: build/sound-drop-release.exe
	cp ./build/sound-drop-release.exe /usr/local/bin/sound-drop
	mkdir -p /usr/local/share/sound-drop-sd
	cp -r ./res /usr/local/share/sound-drop-sd/res
	rm ./build/sound-drop-release.exe

build/sound-drop-release.exe:
	mkdir -p ./build
	$(COMPILER) -std=$(STD) -g -Wall -Wextra  -I./include/ -I$(SOLOUD_HEADERS) -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -L$(SOLOUD_A)  src/glad.c src/stb_image.cpp src/sound-drop.cpp src/Line.cpp src/Ball.cpp src/util.cpp src/Spawner.cpp src/Interactable.cpp src/SaveState.cpp src/StateStack.cpp $(IMGUI_SOURCES) -o build/sound-drop-release.exe -lglfw -lGL -lsoloud_static -lasound


remove:
	rm -rf /usr/local/share/sound-drop-sd
	rm /usr/local/bin/sound-drop


run : build/sound-drop.exe
	./build/sound-drop.exe
