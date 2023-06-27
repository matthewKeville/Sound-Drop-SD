# sound drip

> A physics based beat synthesizer inspired by the original IOS application **Sound Drop** by **Develoe**

Features

---
- Customizable ball spawners 
- An audio sample picker with support for user supplied sounds
- Save slots to experiment with new variations without losing your progress
- An Undo/Redo stack to help dial in your sound
- Musical scale transpositions to change the mood
- Pan & zoom controls to navigate complex pieces
- Adjustable physics parameters
- A simple IMGUI
- Linux/Windows Support

Building from Source

---

## Dependencies

- https://github.com/glfw/glfw
- https://github.com/bkaradzic/GENie


### Linux

- Pull down submodules
> `git init submodules`
> `git submodule update`
- Build soloud using GENie and preferred build tool (I use gmake).
- Build locally
> `make build`

If you want to install sound drip, you can use the install target
> `sudo make install`

Which can be undone with
> `sudo make remove`

### Windows

Building on Windows requires `Build Tools for Visual Studio` to use MSVC (only tested with 2022)

The build process is the same as above, but we specify a different make target.

I build using gmake on Windows so you must install that before building.

I recommend using `chocolatey` > `choco install gmake2` (you may need to add gmake to your path)

> `sudo make --makefile=Makefile.windows`






