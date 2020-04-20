#version 450 core
// START NORMAL VERTEX STUFF
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
out vec3 vNormal;
out vec3 vPos;
out vec2 vTexCoords;
out vec4 vGouraudColor;
uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;

// END NORMAL VERTEX STUFF

// START LIGHTING STUFF
#define LIGHT_POINT 0
#define LIGHT_DIR 1
#define LIGHT_SPOT 2
#define MAX_LIGHTS 16

#define SMODE_PHONG 0
#define SMODE_BLINN 1
#define SMODE_GOURAUD 2

struct Material
{
  sampler2D diffuse;
  sampler2D specular;
  float shininess;
};

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
  float falloffIntensity;
};

#define SPECULAR_STRENGTH 1

out vec4 color;

uniform vec3 viewPos;
uniform Material object;

uniform int shaderMode = SMODE_PHONG;
uniform int lightCount; //# of active lights
uniform UberLight lights[MAX_LIGHTS];
uniform float ZNear = .1;
uniform float ZFar = 300;
uniform vec3 globalAmbient = vec3(.005, .005, .005);

vec3 CalcLocalColor(UberLight light);
float CalcAttenuation(UberLight light);
vec3 CalcDirLight(UberLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(UberLight light, vec3 normal, vec3 viewDir);
vec3 CalcSpotLight(UberLight light, vec3 normal, vec3 viewDir);
// END LIGHTING STUFF




void main()
{
  vTexCoords = aTexCoords;
  vPos = vec3(u_model * vec4(aPos, 1.0));
  vNormal = mat3(transpose(inverse(u_model))) * aNormal; // model->world space
  gl_Position = u_proj * u_view * vec4(vPos, 1.0);

  if (shaderMode == SMODE_GOURAUD)
  {
    vec3 normal = normalize(vNormal);
    vec3 viewDir = normalize(viewPos - vPos);
    vec3 local = vec3(0);

    for (int i = 0; i < lightCount; i++)
    {
      if (lights[i].type == LIGHT_POINT)
        local += CalcPointLight(lights[i], normal, viewDir);
      else if (lights[i].type == LIGHT_DIR)
        local += CalcDirLight(lights[i], normal, viewDir);
      else // LIGHT_SPOT
        local += CalcSpotLight(lights[i], normal, viewDir);
    }
    
    float distance = length(viewPos - vPos); // between view and fragment
    float S = (ZFar - distance) / (ZFar - ZNear);
    S = clamp(S, 0, 1);
    vec3 fog = vec3(.5, .5, .5);
    vec3 final = S * local + (1 - S) * fog;
    vGouraudColor = vec4(final + (globalAmbient * texture(object.diffuse, aTexCoords).xyz), 1.0);
  }
  else
    vGouraudColor = vec4(1.0);
}




vec3 CalcLocalColor(UberLight light, vec3 lightDir, vec3 normal, vec3 viewDir)
{
  // diffuse shading
  float diff = max(dot(normal, lightDir), 0.0);
  
  // specular shading
  float spec = 0;
  if (shaderMode == SMODE_BLINN)
  {
    vec3 halfwayDir = normalize(lightDir + viewDir);
    spec = pow(max(dot(normal, halfwayDir), 0.0), object.shininess) * SPECULAR_STRENGTH;
  }
  else // shaderMode == SMODE_PHONG
  {
    vec3 reflectDir = -lightDir - 2.0 * dot(normal, -lightDir) * normal;
    //vec3 reflectDir = reflect(-lightDir, normal);
    spec = pow(max(dot(viewDir, reflectDir), 0.0), object.shininess) * SPECULAR_STRENGTH;
  }
  
  // combine results
  vec3 ambient  = light.ambient  * vec3(texture(object.diffuse, vTexCoords));
  vec3 diffuse  = light.diffuse  * diff * vec3(texture(object.diffuse, vTexCoords));
  vec3 specular = light.specular * spec * vec3(texture(object.specular, vTexCoords));

  return (ambient + diffuse + specular);// * .0001 + spec;
}


float CalcAttenuation(UberLight light)
{
  float distance = length(light.position - vPos);
  return 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
}


// unaffected by falloff (mimic sun's parallel rays)
vec3 CalcDirLight(UberLight light, vec3 normal, vec3 viewDir)
{
  vec3 lightDir = normalize(-light.direction);
  return CalcLocalColor(light, lightDir, normal, viewDir);
}


// simple point light, affected by falloff
vec3 CalcPointLight(UberLight light, vec3 normal, vec3 viewDir)
{
  vec3 lightDir = normalize(light.position - vPos);
  vec3 local = CalcLocalColor(light, lightDir, normal, viewDir);
  float attenuation = CalcAttenuation(light);
  local *= attenuation;
  return local;
}


// calculates the color when using a spot light.
vec3 CalcSpotLight(UberLight light, vec3 normal, vec3 viewDir)
{
  vec3 lightDir = normalize(light.position - vPos);
  vec3 local = CalcLocalColor(light, lightDir, normal, viewDir);

  // spotlight intensity
  float theta = dot(lightDir, normalize(-light.direction)); 
  float epsilon = light.innerCutOff - light.outerCutOff;
  float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
  float attenuation = CalcAttenuation(light);
  local *= attenuation * pow(intensity, light.falloffIntensity);
  return local;
}