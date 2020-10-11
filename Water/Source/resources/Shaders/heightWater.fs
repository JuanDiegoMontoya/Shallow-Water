#version 450

out vec4 FragColor;

in vec3 vPos;
in vec3 vNormal;
in vec2 vTexCoord;

uniform vec3 u_viewpos;
uniform float u_waterRefract; // water
uniform float u_airRefract;   // air

uniform sampler2D heightTex;
uniform samplerCube envTex;

#define FRESNEL_POWER 5.0


float FresnelSchlick(vec3 i, vec3 n, float Eta, float Power)
{
    float F = ((1.0-Eta) * (1.0-Eta)) / ((1.0+Eta) * (1.0+Eta));
    return F + (1.0 - F) * pow((1.0 - dot(-i, n)), Power);
}


vec3 CalcEnvColor(vec3 normal, vec3 viewDir)
{
    float inDex = u_waterRefract; 
    float exDex = u_airRefract;

    // viewing from under water
    if (dot(normal, -viewDir) > 0)
    {
        float tmp = inDex;
        inDex = exDex;
        exDex = tmp;
        normal *= -1;
    }
    float eta = exDex / inDex;
    float Ratio = FresnelSchlick(-viewDir, normal, eta, FRESNEL_POWER);

    vec3 reflectColor = texture(envTex, reflect(-viewDir, normal)).rgb;
    vec3 refractColor = texture(envTex, refract(-viewDir, normal, eta)).rgb;
    
    // snell's window
    if (all(lessThan(abs(refract(-viewDir, normal, eta)), vec3(0.1))))
        refractColor = reflectColor;
    return mix(refractColor, reflectColor, Ratio);
}


void main()
{
    vec3 viewDir = normalize(u_viewpos - vPos);
    vec3 normal = normalize(vNormal);

    vec3 envColor = CalcEnvColor(normal, viewDir);

    FragColor = vec4(envColor, 1.0);
}