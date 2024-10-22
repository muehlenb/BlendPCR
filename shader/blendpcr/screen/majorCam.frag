#version 330 core

#define CAMERA_NUM 7

in vec2 vScreenPos;

uniform sampler2D color[CAMERA_NUM];
uniform sampler2D vertices[CAMERA_NUM];
uniform sampler2D normals[CAMERA_NUM];
uniform sampler2D depth[CAMERA_NUM];

uniform bool isCameraActive[CAMERA_NUM];

uniform bool useFusion = false;
uniform vec4 cameraVector;
uniform mat4 view;

layout(location = 0) out uint OutIdx;

void main()
{		
	float distanceTreshold = 0.05;

	vec2 halfTexelSize = 2.0 / textureSize(vertices[0], 0);
	
	float mainDistToCam = 9999.0;
	for(int i=0; i < CAMERA_NUM; ++i){
		if(isCameraActive[i]){
			vec4 tVertex = vec4(texture(vertices[i], vScreenPos).xyz, 1.0);
			float tDistToCam = length(tVertex.xyz);
			
			if(tDistToCam >= 0.01 && tDistToCam < mainDistToCam){
				mainDistToCam = tDistToCam;
			}
		}
	}

	float maximumAlpha = 0;
	int maximumAlphaIdx = 0;
	
	float lastDistToCam = 10000;
	
	for(int i=0; i < CAMERA_NUM; ++i){
		if(isCameraActive[i]){
			vec4 vtxTexValue = texture(vertices[i], vScreenPos);
			vec4 currentVertex = vec4(vtxTexValue.xyz, 1.0);
			
			float currentAlpha = vtxTexValue.a;
			
			float distToCam = length(currentVertex.xyz);
						
			if(!(distToCam >= 0.01) || !(currentAlpha > 0.0) || abs(mainDistToCam - distToCam) > 0.05)
				continue;
				
			if(distToCam + distanceTreshold < lastDistToCam){
				maximumAlpha = currentAlpha;
				maximumAlphaIdx = i;
				
				lastDistToCam = distToCam;
			} else if(distToCam - distanceTreshold > lastDistToCam){
				continue;
			} else if(currentAlpha > maximumAlpha){
				maximumAlpha = currentAlpha;
				maximumAlphaIdx = i;
			}
		}
	}

	OutIdx = uint(maximumAlphaIdx);
}
