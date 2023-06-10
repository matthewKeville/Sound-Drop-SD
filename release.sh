# this is just a template for what a final .deb package will look like.
# this has exposed problems with my current code. It assumes paths local to
# the source code directory. Whereas any given installation my have different paths
# on debian systems I would put installed image files at /usr/local/share/myprogram/the-image.png
# but the source code doesn't take this into account, and it would differ on windows...

NAME=SoundDropSD
VERSION=0.1
REVISION=0.1
ARCH=x86_64

PREFIX=release/"$NAME"_"$VERSION"_"$REVISION"_"$ARCH"
make clean && make

mkdir -p "$PREFIX"

mkdir -p "$PREFIX"/usr/local/bin
cp build/sound-drop.exe "$PREFIX"/usr/local/bin/sound-drop.exe

mkdir -p "$PREFIX"/usr/local/share/"$NAME"/res
mkdir -p "$PREFIX"/usr/local/share/"$NAME"/shaders

cp res/* "$PREFIX"/usr/local/share/"$NAME"/res
cp shaders/* "$PREFIX"/usr/local/share/"$NAME"/shaders

mkdir -p "$PREFIX"/DEBIAN
cp control "$PREFIX"/DEBIAN/control

dpkg-deb --build --root-owner-group $PREFIX

