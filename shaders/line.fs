#version 330 core

uniform vec3 Color;
out vec4 FragColor;

void main()
{
  FragColor = vec4(Color.x,Color.y,Color.z,1.0);
}
