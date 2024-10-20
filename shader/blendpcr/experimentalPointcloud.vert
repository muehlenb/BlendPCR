#version 330 core

layout (location = 0) in vec3 vInPos;   // the position attribute

uniform sampler2D texture2D_vertices;
uniform sampler2D texture2D_edgeNearness;
uniform sampler2D texture2D_colors;
uniform sampler2D texture2D_normals;
uniform sampler2D texture2D_overlap;

out vec4 vPos;
out vec4 vColor;
out vec4 vCamPos;
out float vEdgeDistance;
out vec2 vCamPosAlpha;
out vec3 vNormal;
out vec2 vTexCoord;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
	// Debugging Code:
    //gl_Position = projection * view * model * vec4(vInPos, 1.0);
	//vColor =  texture(texture2D_vertices, vInPos.xy);
	
	vec2 halfTexelSize = 0.5 / textureSize(texture2D_vertices, 0);
	
	vec4 rawCamPos = texture(texture2D_vertices, vInPos.xy + halfTexelSize);
	
	vCamPos = vec4(rawCamPos.xyz, 1.0);
	vCamPosAlpha = texture(texture2D_overlap, vInPos.xy + halfTexelSize).rg;
	
	vEdgeDistance = texture(texture2D_edgeNearness, vInPos.xy + halfTexelSize).r;
	vColor = texture(texture2D_colors, vInPos.xy + halfTexelSize);
	vNormal = texture(texture2D_normals, vInPos.xy + halfTexelSize).xyz;
	
	vTexCoord = vInPos.xy + halfTexelSize;
	
	vPos = view * model * vCamPos;
    gl_Position = projection * vPos;
}