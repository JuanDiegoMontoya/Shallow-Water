#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

//out vec3 vNormal;
//out vec3 vPos;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;

void main()
{
  vec3 vPos = vec3(u_model * vec4(aPos, 1.0));
  //vNormal = mat3(transpose(inverse(u_model))) * aNormal; // model->world space
  gl_Position = u_proj * u_view * vec4(vPos, 1.0);
}