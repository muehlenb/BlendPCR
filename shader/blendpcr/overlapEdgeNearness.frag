#version 330 core

#define CAMERA_NUM 7

uniform sampler2D overlap[CAMERA_NUM];

uniform bool useFusion = false;
uniform int cameraID = -1;

uniform int kernelRadius = 10;

in vec2 vScreenPos;

out vec4 FragColor;

void main()
{	
	vec2 texelSize = 1.0 / textureSize(overlap[cameraID], 0);
		
	float edgeVal = texture(overlap[cameraID], vScreenPos).r;
	if(edgeVal < 0.25){
		FragColor = vec4(1.0, 0.0, 0.0, 0.0);
		return;
	}
	
	float maxInfluence = 1.0;
	
	int rad = 10;
	
	for(int dX = -rad; dX <= rad; ++dX){
		for(int dY = -rad; dY <= rad; ++dY){
			vec2 offset = vec2(dX, dY) * texelSize;
			vec2 coord = vScreenPos + offset;
			float value = texture(overlap[cameraID], coord).r;
			
			if(value < 0.25){
				float d = clamp((float(dX) * float(dX) + float(dY) * float(dY)) / (rad * rad), 0.0, 1.0);
				if(!isnan(d))
					maxInfluence = min(maxInfluence, d);
			}
		}
	}

	FragColor = vec4(maxInfluence, maxInfluence, maxInfluence, 1.0);
}