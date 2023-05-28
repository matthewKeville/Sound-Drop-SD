#version 330 core
layout (location = 0) in vec3 aPos;

uniform vec2 WorldPosition;

void main()
{
  gl_Position = vec4(aPos.x + WorldPosition.x,aPos.y + WorldPosition.y,aPos.z,1.0);
}

