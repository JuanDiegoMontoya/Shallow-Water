#version 450

// sunlight
struct Light
{
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;

  vec3 direction;
};

out vec4 FragColor;

in vec3 vPos;
in vec3 vNormal;
in vec2 vTexCoord;

uniform vec3 u_color;
uniform vec3 u_viewpos;

uniform sampler2D heightTex;
uniform samplerCube envTex;
uniform vec3 objDiff = vec3(.31, .83, .86);
uniform float objSpec = 1.0;
uniform Light sun;

#define SHININESS 64.0
#define FRESNEL_POWER 5.0


float FresnelSchlick(vec3 i, vec3 n, float Eta, float Power)
{
	float F = ((1.0-Eta) * (1.0-Eta)) / ((1.0+Eta) * (1.0+Eta));
	return F + (1.0 - F) * pow((1.0 - dot(-i, n)), Power);
}


vec3 CalcEnvColor(vec3 normal, vec3 viewDir)
{
	vec3 reflectColor = texture(envTex, reflect(-viewDir, normal)).rgb;
	float inDex = 1.33; // water
	float exDex = 1;   // air
	float eta = exDex / inDex;
	vec3 refractColor = texture(envTex, refract(-viewDir, normal, eta)).rgb;

    float Ratio = FresnelSchlick(-viewDir, normal, eta, FRESNEL_POWER);
	return mix(refractColor, reflectColor, Ratio);
}


vec3 CalcLocalColor(Light light, vec3 lightDir, vec3 normal, vec3 viewDir)
{
	// diffuse shading
	float diff = max(dot(normal, lightDir), 0.0);

	// specular shading
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), SHININESS);

	// combine results
	vec3 ambient  = light.ambient * objDiff;
	vec3 diffuse  = light.diffuse * diff * objDiff;
	vec3 specular = light.specular * spec * objSpec;
	return (ambient + diffuse + specular);
}


// unaffected by falloff (mimic sun's parallel rays)
vec3 CalcDirLight(Light light, vec3 normal, vec3 viewDir)
{
	vec3 lightDir = normalize(-light.direction);
	return CalcLocalColor(light, lightDir, normal, viewDir);
}


// phong
void main()
{
	vec3 viewDir = normalize(u_viewpos - vPos);
	vec3 normal = normalize(vNormal);

	vec3 phongColor = CalcDirLight(sun, normal, viewDir);
	vec3 envColor = CalcEnvColor(normal, viewDir);
	vec3 finalColor = mix(phongColor, envColor, .999);

	FragColor = vec4(finalColor, 1.0);
}