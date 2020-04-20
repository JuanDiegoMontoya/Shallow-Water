#version 450 core

struct Material
{
  //sampler2D diffuse;
  //sampler2D specular;
  //float     shininess;
  vec3 diffuse;
  float specular;
};

/*
// allows lights to be any type
struct UberLight
{
  // all light
  int type;
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;

  // point + spot light
  vec3 position;
  float constant;
  float linear;
  float quadratic;

  // directional + spot light
  vec3 direction;

  // spot light
  float innerCutOff;
  float outerCutOff;
};
*/

out vec4 color;

//uniform vec3 viewPos;
uniform Material object;

//in vec3 vNormal;
//in vec3 vPos;
//in vec2 vTexCoords;

void main()
{
  color = vec4(object.diffuse, 1.0);
}