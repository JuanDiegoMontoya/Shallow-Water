#version 450 core
struct dirLight
{
  vec3 pos;
  vec3 color;
  //vec3 dir;
};

struct material
{
  vec3 color;
  float specular;
};

out vec4 FragColor;

in flat vec3 vNormal;
in flat vec3 vPos;

uniform dirLight sun;
uniform material obj;
uniform vec3 viewPos;
uniform bool useObjColor = true;

void main()
{
  vec3 actualColor;
  if (useObjColor)
    actualColor = obj.color;
  else
    actualColor = vNormal * .5 + .5;
  
  // ambient
  float ambientStrength = 0.1;
  vec3 ambient = ambientStrength * sun.color;
  
  // diffuse
  vec3 norm = normalize(vNormal);
  vec3 lightDir = normalize(sun.pos - vPos);
  float diff = max(dot(norm, lightDir), 0.0);
  vec3 diffuse = diff * sun.color;
  
  // specular (shininess)
  float specularStrength = 0.5;
  vec3 viewDir = normalize(viewPos - vPos);
  vec3 reflectDir = reflect(-lightDir, norm);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), obj.specular);
  vec3 specular = specularStrength * spec * sun.color;
  
  vec3 result = (ambient + diffuse + specular) * actualColor;
  FragColor = vec4(result, 1.0);
}