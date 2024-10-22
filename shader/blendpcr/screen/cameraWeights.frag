#version 330 core

#define CAMERA_NUM 7

in vec2 vScreenPos;

uniform usampler2D dominanceTexture;

uniform bool isCameraActive[CAMERA_NUM];

layout(location = 0) out vec4 FragColor1;
layout(location = 1) out vec4 FragColor2;



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
	float distanceTreshold = 0.05;

	float result[8];
	for(int i=0; i < 8; ++i){
		result[i] = 0;
	}
	
	vec2 halfTexelSize = 1.0 / textureSize(dominanceTexture, 0);
	
	float count = 0;

	for(int y = -10; y <= 10; y += 1){
		for(int x = -10; x <= 10; x += 1){
			vec2 currentScreenPos = vScreenPos + halfTexelSize * vec2(x,y);
			uint dominantCam = texture(dominanceTexture, currentScreenPos).r;
		
			for(int i=0; i < 8; ++i){
				result[i] += dominantCam == uint(i) ? 1.0 : 0.0;
			}
			++count;
		}
	}

	
	FragColor1 = vec4(result[0] / count, result[1] / count, result[2] / count, result[3] / count);
	FragColor2 = vec4(result[4] / count, result[5] / count, result[6] / count, result[7] / count);
}
