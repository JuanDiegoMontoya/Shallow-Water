#version 450 core

#define LIGHT_POINT 0
#define LIGHT_DIR 1
#define LIGHT_SPOT 2
#define MAX_LIGHTS 16

#define SMODE_PHONG 0
#define SMODE_BLINN 1
#define SMODE_GOURAUD 2

#define TEXENT_POSITION 0
#define TEXENT_NORMAL 1

#define PROJ_CUBE 0
#define PROJ_SPHERE 1
#define PROJ_CYLINDER 2

#define PI     3.14159265359
#define TWO_PI 6.28318530718

#define EMODE_REFLECT 0
#define EMODE_REFRACT 1
#define EMODE_FRESNEL 2

struct Material
{
  sampler2D diffuse;
  sampler2D specular;
  float shininess;
};

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
  float falloffIntensity;
};

#define SPECULAR_STRENGTH 1

out vec4 color;

uniform vec3 viewPos;
uniform Material object;

// this stuff should be in a buffer block
uniform int shaderMode = SMODE_PHONG;
uniform int lightCount; // # of active lights
uniform UberLight lights[MAX_LIGHTS];
uniform float ZNear = .1;
uniform float ZFar = 300;
uniform vec3 globalAmbient = vec3(.005, .005, .005);

// texcoord generation
uniform int projector;
uniform int texEntity;
uniform bool GPUTexCoords;
uniform vec3 minbox; // min bounding coord
uniform vec3 maxbox; // max bounding coord
uniform vec3 center; // center point of bounding box

// environment mapping
/*
uniform sampler2D env0;
uniform sampler2D env1;
uniform sampler2D env2;
uniform sampler2D env3;
uniform sampler2D env4;
uniform sampler2D env5;
*/
uniform sampler2D env[6];
uniform vec3 exDex; // exterior (air) index of refraction
uniform vec3 inDex; // interior index of refraction
uniform int mapMode = EMODE_REFRACT;
uniform float emissiveRatio = 0; // 0-1, 0=purely transparent, 1=purely phongy
uniform float fresnelPower = 5.0;
uniform bool viewRatio = false;

in vec3 vNormal;
in vec3 vPos;
in vec2 vTexCoords;
in vec4 vGouraudColor;

vec2 finalTexCoords; // global

vec3 CalcLocalColor(UberLight light);
float CalcAttenuation(UberLight light);
vec3 CalcDirLight(UberLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(UberLight light, vec3 normal, vec3 viewDir);
vec3 CalcSpotLight(UberLight light, vec3 normal, vec3 viewDir);

vec2 CalcTexCoords();
vec2 FakeAssSampleCube(const vec3 v, out float faceIndex);
float FresnelSchlick(vec3 i, vec3 n, float Eta, float Power);
vec3 NotRefract(in vec3 I, in vec3 N, float eta);
vec3 NotReflect(in vec3 I, in vec3 N);


void main()
{
  if (GPUTexCoords == true)
    finalTexCoords = CalcTexCoords();
  else
    finalTexCoords = vTexCoords;

  vec4 phongColor;

  // properties
  if (shaderMode == SMODE_PHONG || shaderMode == SMODE_BLINN)
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
    
    // calculate final color (with fog and global ambient)
    float distance = length(viewPos - vPos); // between view and fragment
    float S = (ZFar - distance) / (ZFar - ZNear);
    S = clamp(S, 0, 1);
    vec3 fog = vec3(.5, .5, .5);
    vec3 final = S * local + (1 - S) * fog;
    phongColor = vec4(final + (globalAmbient * texture(object.diffuse, finalTexCoords).xyz), 1.0);
  }
  else// if (shaderMode == SMODE_GOURAUD)// (we were passed color from vertex shader)
  {
    phongColor = vGouraudColor;
  }

  vec3 normal = normalize(vNormal);
  vec3 viewDir = normalize(viewPos - vPos);

  // reflection color
  float LFaceIndex;
  vec3 L = NotReflect(-viewDir, normal);
  vec2 LC = FakeAssSampleCube(L, LFaceIndex);
  vec4 reflectColor = texture(env[int(LFaceIndex)], LC);

  // refraction color (add r,g,b components)
  vec4 transColor = vec4(0);
  for (int i = 0; i < 3; i++)
  {
    float eta = exDex[i] / inDex[i];
    vec3 R = NotRefract(-viewDir, normal, eta);

    float RFaceIndex;
    vec2 RC = FakeAssSampleCube(R, RFaceIndex);
    //RC.x = 1.0 - RC.x;

    float Ratio = FresnelSchlick(-viewDir, normal, eta, fresnelPower);
    if (mapMode == EMODE_REFRACT)
      Ratio = 0.0;
    else if (mapMode == EMODE_REFLECT)
      Ratio = 1.0;

    vec4 refractColor = texture(env[int(RFaceIndex)], RC);

    // combine
    if (viewRatio == false)
    {
      transColor[i] += mix(refractColor[i], reflectColor[i], Ratio);
      transColor.a += mix(refractColor.a, reflectColor.a, 0.5) / 3.0; // avg of alphas
    }
    else
    {
      transColor[i] += (Ratio);
      transColor.a += Ratio / 3.0;
    }
  }

  //color = refractColor;
  color = mix(transColor, phongColor, emissiveRatio);
  
  //transColor *= .01;
  //color += vec4(envCoords, 1, 1);
  //color += .25 * vec4(.5 * reflected + .5, 1);
  //color += vec4(.5 * refracted + .5, 1);
  //color = .001 * color + texture(object.diffuse, finalTexCoords);
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
  vec3 ambient  = (light.ambient) * vec3(texture(object.diffuse, finalTexCoords));
  vec3 diffuse  = (light.diffuse)  * diff * vec3(texture(object.diffuse, finalTexCoords));
  vec3 specular = light.specular * spec * vec3(texture(object.specular, finalTexCoords));
  if (light.type == LIGHT_SPOT)
    ambient = vec3(0);
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
  vec3 lightDir = normalize(light.position - vPos); vec3 local =
  CalcLocalColor(light, lightDir, normal, viewDir);

  // spotlight intensity 
  float theta = dot(lightDir,normalize(-light.direction));
  float epsilon = light.innerCutOff - light.outerCutOff; 
  float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0); 
  float attenuation = CalcAttenuation(light);
  local *= attenuation * pow(intensity, light.falloffIntensity); return local;
}


vec2 CalcTexCoords()
{ 
  vec2 tempTexCoords;
  switch (projector)
  {
  case PROJ_CUBE:
    {
      vec3 absVec;
      if (texEntity == TEXENT_NORMAL)
        absVec = abs(vNormal);
      else // TEXENT_POSITION
        absVec = abs(vPos);
      vec2 UV;

      // +-X
      if (absVec.x >= absVec.y && absVec.x >= absVec.z)
      {
        if (vPos.x < 0.0)
          (UV.s = vPos.z);
        else 
          (UV.s = -vPos.z);
        UV.t = vPos.y;
        UV.s /= maxbox.z;
        UV.t /= maxbox.y;
        //UV.s /= absVec.x;
        //UV.t /= absVec.x;
      }
      // +-Y
      else if (absVec.y >= absVec.x && absVec.y >= absVec.z)
      {
        if (vPos.y < 0.0)
          (UV.t = vPos.z);
        else
          (UV.t = -vPos.z);
        UV.s = vPos.x;
        UV.s /= maxbox.x;
        UV.t /= maxbox.z;
      }
      // +-Z
      else
      {
        if (vPos.z < 0.0)
          (UV.s = -vPos.x);
        else
          (UV.s = vPos.x);
        UV.t = vPos.y;
        UV.s /= maxbox.x;
        UV.t /= maxbox.y;
      }

      UV = (UV + 1.f) / 2.f;
      tempTexCoords = UV;
    }
    break;
  case PROJ_SPHERE:
    {
      vec3 tpos;
      if (texEntity == TEXENT_POSITION)
        tpos = vPos - center;
      else if (texEntity == TEXENT_NORMAL)
        tpos = vNormal;
      float theta = atan(tpos.y / tpos.x);
      float Z = (tpos.z - minbox.z) / (maxbox.z - minbox.z);
      float r = length(tpos);
      float phi = acos(tpos.z / r);
      float U = theta / TWO_PI; // convert to 0-1 range
      float V = (PI - phi) / (PI);
      tempTexCoords = vec2(U, V);
    }
    break;
  case PROJ_CYLINDER:
    {
      vec3 tpos;
      if (texEntity == TEXENT_POSITION)
        tpos = vPos - center;
      else if (texEntity == TEXENT_NORMAL)
        tpos = vNormal;
      float theta = atan(tpos.y / tpos.x);
      float Z = (tpos.z - minbox.z) / (maxbox.z - minbox.z);
      float U = theta / TWO_PI; // convert to 0-1 range
      float V = Z;
      tempTexCoords = vec2(U, V);
    }
    break;
  default:
    break;
  }
  return tempTexCoords;
}


vec2 FakeAssSampleCube(
    const vec3 v,
    out float faceIndex)
{
  vec3 vAbs = abs(v);
  float ma;
  vec2 uv;
  if(vAbs.z >= vAbs.x && vAbs.z >= vAbs.y)
  {
    faceIndex = v.z < 0.0 ? 4.0 : 5.0;
    ma = 0.5 / vAbs.z;
    uv = vec2(v.z < 0.0 ? -v.x : v.x, -v.y);
  }
  else if(vAbs.y >= vAbs.x)
  {
    faceIndex = v.y < 0.0 ? 3.0 : 2.0;
    ma = 0.5 / vAbs.y;
    uv = vec2(v.x, v.y < 0.0 ? -v.z : v.z);
  }
  else
  {
    faceIndex = v.x < 0.0 ? 1.0 : 0.0;
    ma = 0.5 / vAbs.x;
    uv = vec2(v.x < 0.0 ? v.z : -v.z, -v.y);
  }
  return uv * ma + 0.5;
}


float FresnelSchlick(vec3 i, vec3 n, float Eta, float Power)
{
  float F = ((1.0-Eta) * (1.0-Eta)) / ((1.0+Eta) * (1.0+Eta));
  return F + (1.0 - F) * pow((1.0 - dot(-i, n)), Power);
}


vec3 NotReflect(in vec3 I, in vec3 N)
{
  return I - 2.0 * dot(N, I) * N;
}


vec3 NotRefract(in vec3 I, in vec3 N, float eta)
{
  vec3 R;
  float k = 1.0 - eta * eta * (1.0 - dot(N, I) * dot(N, I));
  if (k < 0.0)
    R = vec3(0.0);
  else
    R = eta * I - (eta * dot(N, I) + sqrt(k)) * N;
  return R;
}