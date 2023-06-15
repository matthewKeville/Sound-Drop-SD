SOLOUD_HEADERS = /home/galaxy/code/foss/soloud/include
SOLOUD_SO = /home/galaxy/code/foss/soloud/lib/
SOLOUD_A = /home/galaxy/code/foss/soloud/lib/

IMGUI_DIR = imgui
IMGUI_SOURCES =  $(IMGUI_DIR)/imgui.cpp  $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp 
IMGUI_SOURCES += $(IMGUI_DIR)/imgui_widgets.cpp
IMGUI_SOURCES += $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp

all: build/sound-drop.exe 
build/sound-drop.exe: build/
	g++ -g -Wall -Wextra  -I./include/ -I$(SOLOUD_HEADERS) -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -L$(SOLOUD_A)  src/glad.c src/stb_image.cpp src/sound-drop.cpp src/Line.cpp src/Ball.cpp src/util.cpp src/Spawner.cpp src/Interactable.cpp src/SaveState.cpp $(IMGUI_SOURCES) -o build/sound-drop.exe -lglfw -lGL -lsoloud_static -lasound
build/ :
	mkdir build
clean : 
	rm -rf build
	rm -rf release
run : build/sound-drop.exe
	./build/sound-drop.exe
