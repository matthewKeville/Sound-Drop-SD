# sound drop sd

> A physics based beat synthesizer inspired by the original IOS application **Sound Drop** by **Develoe**

In sound drop sd you create beats by bouncing balls on lines.
The length of the line determines what notes are played.


[<img src="showcase/snap1.png" width="300" />](showcase/snap1.png)
[<img src="showcase/snap2.png" width="300" />](showcase/snap2.png)

Features

---
- Customizable ball spawners 
- An audio sample picker with support for user supplied samples
- Save slots to experiment with new variations without losing your progress
- An Undo/Redo stack to help dial in your sounds
- Musical scale transpositions to change the mood
- Color and notes mapped to the width of lines
- Pan & zoom controls to navigate complex pieces
- Adjustable physics parameters
- A simple IMGUI
- Keyboard shortcuts
- Linux/Windows Support

Building from Source

---

Dependencies

- https://github.com/glfw/glfw
- https://github.com/bkaradzic/GENie

---

### Linux

> Pull down git submodules
- `git init submodules`
- `git submodule update`
> Build soloud using GENie and gmake
- `cd soloud/build`
- `genie gmake`
- `cd gmake; make`
> Build locally
- `cd ../../../`
- `make build`

> (Optional) Install
- `sudo make install`
> Which can be undone with
- `sudo make remove`

* I assume you have installed the following packages from apt or equivalent from other package managers
`libglfw3-dev libglfw3`

---

### Windows

Building on Windows requires `Build Tools for Visual Studio 2022` for MSVC.
The build process is nearly identical.

> Acquire glfw3 ( install in this repository )
- `nuget install glfw` ( or equivalent )
> Open a `Visual Studio Developer Command Prompt and build using the windows makefile
- `sudo make --makefile=Makefile.windows`

* if you don't have gmake I recommend installing it with chocolately
- `choco install gmake2`
> Note, you need to add this to your path

---

Running the application

---

`./build/sound-drop.exe`




