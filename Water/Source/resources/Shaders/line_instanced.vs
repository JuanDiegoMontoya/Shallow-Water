#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in mat4 aModel;

out vec3 vColor;

uniform mat4 u_view;
uniform mat4 u_proj;

void main()
{
  vColor = aColor;
  gl_Position = u_proj * u_view * (aModel * vec4(aPos, 1.0));
}