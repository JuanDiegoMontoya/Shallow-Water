#version 450
layout (location = 0) in vec2 aPosXZ;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 u_proj;
uniform mat4 u_view;
uniform mat4 u_model;

out vec3 vPos;
out vec3 vNormal;
out vec2 vTexCoord;

uniform sampler2D heightTex; // height field
//uniform vec2 dim; // dimensions of heightTex

void main()
{
	vTexCoord = aTexCoord;
	vec2 dim = textureSize(heightTex, 0);
	float height = texture(heightTex, aTexCoord).r;
	float uHeight = texture(heightTex, vec2(aTexCoord.x, aTexCoord.y + 1.0 / dim.y)).r;
	float rHeight = texture(heightTex, vec2(aTexCoord.x + 1.0 / dim.x, aTexCoord.y)).r;
	vec3 aPos = vec3(aPosXZ.x, height, aPosXZ.y);
	//if (aTexCoord.y > .5)
	//	aPos.y = 100;
	vec3 aPosU = vec3(aPosXZ.x, uHeight, aPosXZ.y + 1);
	vec3 aPosR = vec3(aPosXZ.x + 1, rHeight, aPosXZ.y);
	vec3 aNormal = cross(aPosU - aPos, aPosR - aPos);
	vPos = vec3(u_model * vec4(aPos, 1.0));
	vNormal = mat3(transpose(inverse(u_model))) * aNormal; // model->world space
	//vNormal = aNormal;
	gl_Position = u_proj * u_view * vec4(vPos, 1.0);
}