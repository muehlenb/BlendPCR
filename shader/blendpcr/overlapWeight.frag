#version 330 core

#define CAMERA_NUM 7

uniform usampler2D overlap[CAMERA_NUM];

uniform bool useFusion = false;
uniform int cameraID = -1;

uniform int kernelRadius = 10;
uniform bool isCameraActive[CAMERA_NUM];

in vec2 vScreenPos;

out uvec4 FragColor;

float calculateTheta(float d){	
	return pow(2.71828, -(d*d) / (10*10));
}

float max4(vec4 v) {
	return max(max(v.x, v.y), max(v.z, v.w));
}

uvec4 encode8Bytes(float inpt[8]) {
    uvec4 result;

    result.x = (uint(inpt[0] * 255.0) & 0xFFu) | ((uint(inpt[1] * 255.0) & 0xFFu) << 8);
    result.y = (uint(inpt[2] * 255.0) & 0xFFu) | ((uint(inpt[3] * 255.0) & 0xFFu) << 8);
    result.z = (uint(inpt[4] * 255.0) & 0xFFu) | ((uint(inpt[5] * 255.0) & 0xFFu) << 8);
    result.w = (uint(inpt[6] * 255.0) & 0xFFu) | ((uint(inpt[7] * 255.0) & 0xFFu) << 8);

    return result;
}


void main()
{		
	vec2 texelSize = 1.0 / textureSize(overlap[cameraID], 0);
	
	int rad = 10;
	
	float influence[8];
	float sumWeight[8]; 
		
	for(int dX = -rad; dX <= rad; ++dX){
		for(int dY = -rad; dY <= rad; ++dY){
			vec2 offset = vec2(dX, dY) * texelSize;
			vec2 coord = vScreenPos + offset;
				
			uint overlp = texture(overlap[cameraID], coord).r;
			
			if(overlp == 255u)
				continue;
				
			for(uint i=0u; i < uint(CAMERA_NUM); ++i){
				if(uint(cameraID) == i || !isCameraActive[i])
					continue;
						
				influence[i] += (overlp == i ? 1.0 : 0.0);
				++sumWeight[i];
			}
		}
	}
	
	float results[8];
	for(int i=0; i < 8; ++i){ 
		if(cameraID == i || !isCameraActive[i] || i >= CAMERA_NUM){
			results[i] = 0.0;
		} else {
			results[i] = influence[i] / sumWeight[i];
		}
	}
	
	FragColor = encode8Bytes(results);
}