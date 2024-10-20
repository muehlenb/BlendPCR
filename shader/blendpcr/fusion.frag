#version 330 core

#define CAMERA_NUM 7

in vec2 vScreenPos;

uniform sampler2D vertices[CAMERA_NUM];
uniform sampler2D normals[CAMERA_NUM];
uniform sampler2D lookup[CAMERA_NUM];
uniform usampler2D overlapWeight[CAMERA_NUM];

uniform bool useFusion = false;
uniform int cameraID = -1;

uniform mat4 fromCurrentCam[CAMERA_NUM]; // previously: cam1ToCam2
uniform mat4 toCurrentCam[CAMERA_NUM]; // previously: cam2ToCam1

uniform bool isCameraActive[CAMERA_NUM];
uniform mat4 cameraMatrix[CAMERA_NUM];

layout (location = 0) out vec4 outPC;
layout (location = 1) out vec4 outNormal;

bool getImageCoordByPoint(in vec3 p, out vec2 result, int camID){
	float x = ((p.x / p.z) * 0.5 + 0.5);
	float y = ((p.y / p.z) * 0.5 + 0.5);
	
	if(x < 0 || x > 1 || y < 0 || y > 1)
		return false;
	
	result = texture(lookup[camID], vec2(x, y)).xy;
	
	return true;
}

float decodeFloat(uvec4 encoded, int index) {
    uint packedValue = encoded[index / 2];

    uint offset = 8u * (uint(index) % 2u);
    
    // Extrahiere den 8-Bit-Wert und konvertiere ihn zu float
    uint extractedValue = (packedValue >> offset) & 0xFFu;
    return float(extractedValue) / 255.0;
}


void main()
{	
	float ownWeight = 1.0;
	
	vec3 currentCamNormal = texture(normals[cameraID], vScreenPos).xyz;
	vec3 currentWorldNormal = (cameraMatrix[cameraID] * vec4(currentCamNormal, 0.0)).xyz;
	vec3 currentPoint = texture(vertices[cameraID], vScreenPos).xyz;
	
	if(!useFusion){	
		outPC = vec4(currentPoint, ownWeight);
		//outRGB = vec4(cameraID==3?1.0:0.0, cameraID==1?1.0:0.0, cameraID==0?1.0:0.0, 1.0);
		
		//outRGB = vec4(texture(color[cameraID], vScreenPos).rgb, ownWeight);
		outNormal = vec4(currentWorldNormal, 0.0);
		return;
	}
	
	for(int i=0; i < CAMERA_NUM; ++i){
		if(cameraID == i || !isCameraActive[i])
			continue;
	
		int otherCamID = i;
		
		vec3 currentPoint_OtherCamSpace = (fromCurrentCam[otherCamID] * vec4(currentPoint, 1)).xyz;
		vec2 otherCamImageCoords;
		bool overlappingArea = getImageCoordByPoint(currentPoint_OtherCamSpace, otherCamImageCoords, otherCamID);

		if(overlappingArea){
			vec3 otherCamPoint = texture(vertices[otherCamID], otherCamImageCoords).xyz;
			float dist = distance(otherCamPoint, currentPoint_OtherCamSpace);
					
			vec3 otherCamNormal = texture(normals[otherCamID], otherCamImageCoords).xyz;
					
			float normalAngle = dot(normalize(vec4(currentCamNormal, 0.0)), normalize(toCurrentCam[otherCamID] * vec4(otherCamNormal, 0.0)));
					
			if(!isnan(otherCamPoint.x) && dist < 0.04 && normalAngle > 0){
				float otherFactor = decodeFloat(texture(overlapWeight[cameraID], vScreenPos),otherCamID);
				float currentFactor = decodeFloat(texture(overlapWeight[otherCamID], otherCamImageCoords),cameraID);
				
				// Annäherung beider Maps:
				otherFactor = otherFactor * 0.5 + (1 - currentFactor) * 0.5;
				
				// Interpolation der Kanten:
				//otherFactor = eD.b * otherFactor + (1 - eD.b) * (eD.g);

				vec3 otherPoint_InCurrentCam = (toCurrentCam[otherCamID] * vec4(otherCamPoint, 1)).xyz;
				
				outPC = vec4(currentPoint * (1-otherFactor) + otherPoint_InCurrentCam * otherFactor, 1 - otherFactor);

				vec2 fixedCurrentTexCoords;
				vec2 fixedOtherTexCoords;

				getImageCoordByPoint(outPC.xyz, fixedCurrentTexCoords, cameraID);
				getImageCoordByPoint(otherCamPoint.xyz, fixedOtherTexCoords, otherCamID);

				//vec3 currentColor = texture(color[cameraID], fixedCurrentTexCoords).rgb;
				//vec3 otherCamColor = texture(color[otherCamID], fixedOtherTexCoords).rgb;
				//vec3 currentColor = vec3(cameraID==3?1.0:0.0, cameraID==1?1.0:0.0, cameraID==0?1.0:0.0);
				//vec3 otherCamColor = vec3(otherCamID==3?1.0:0.0, otherCamID==1?1.0:0.0, otherCamID==0?1.0:0.0);

				//outRGB = vec4(currentColor * (1-otherFactor) + otherCamColor * otherFactor, 0.0);	
				////outRGB = vec4(currentColor, ownWeight);
				
				vec3 otherWorldNormal = (cameraMatrix[otherCamID] * vec4(otherCamNormal, 0.0)).xyz;	
				outNormal = vec4(normalize(currentWorldNormal * (1-otherFactor) + otherWorldNormal * otherFactor), 0.0);	
				
				float g = cameraID == 0 ? 1 - otherFactor : otherFactor;
				float b = cameraID == 0 ? otherFactor : 1 - otherFactor;
				
				////outRGB = vec4(eD.g, eD.g, eD.g, 0.0);
				
				////outRGB = vec4(0.0, g ,b, 0.0);
				
				return;
			}
		}
	}
	
	//vec3 currentColor = texture(color[cameraID], vScreenPos).rgb;
	
	//vec3 currentColor = vec3(cameraID==3?1.0:0.0, cameraID==1?1.0:0.0, cameraID==0?1.0:0.0);
	
	outPC = vec4(currentPoint, 1);
	//outRGB = vec4(currentColor, ownWeight);
	outNormal = vec4(currentWorldNormal, 0.0);
}
