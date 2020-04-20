#version 450
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 u_proj;
uniform mat4 u_view;
uniform mat4 u_model;

out vec3 vPos;
out vec3 vNormal;

void main()
{
  vPos = vec3(u_model * vec4(aPos, 1.0));
  vNormal = mat3(transpose(inverse(u_model))) * aNormal; // model->world space
  //vNormal = aNormal;
  gl_Position = u_proj * u_view * vec4(vPos, 1.0);
}