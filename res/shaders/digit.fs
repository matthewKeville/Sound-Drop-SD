#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

uniform vec3 Color;
uniform sampler2D Texture0;

void main()
{
  FragColor = mix(texture(Texture0, TexCoord), vec4(Color.x,Color.y,Color.z,1.0), 0.0f);
  //FragColor = vec4(1.0,0.0f,1.0f,1.0);
}
