#version 330 core

#define CAMERA_NUM 7

in vec2 vScreenPos;

uniform sampler2D vertices[CAMERA_NUM];
uniform sampler2D normals[CAMERA_NUM];
uniform sampler2D edgeDistances[CAMERA_NUM];
uniform sampler2D lookup[CAMERA_NUM];

uniform mat4 fromCurrentCam[CAMERA_NUM]; // previously: cam1ToCam2
uniform mat4 toCurrentCam[CAMERA_NUM]; // previously: cam2ToCam1

uniform bool isCameraActive[CAMERA_NUM];

uniform bool useFusion = false;
uniform int cameraID = -1;

out vec2 FragResult;

float easeInOut(float x){
	return 3*x*x + 2*x*x*x;
}


void main()
{		
	float maxCamDist = 6.0;

	vec3 point = texture(vertices[cameraID], vScreenPos).xyz;
	vec3 normal = texture(normals[cameraID], vScreenPos).xyz;
	float edgeProximity = texture(edgeDistances[cameraID], vScreenPos).r;
	
	float camDist = clamp(length(point), 0.0, maxCamDist);
	
	//Irgendwie dafür sorgen, dass möglichst immer nur eine Kamera relevant ist, indem Gewicht nichtlinear skaliert wird!!!!
	
	float distFactor = (maxCamDist * maxCamDist - camDist * camDist)/36 * clamp(dot(normalize(normal), normalize(point)), 0.1, 1.0);
	
	FragResult = vec2(distFactor, easeInOut(clamp(1.0-edgeProximity,0.0,1.0)));
}
