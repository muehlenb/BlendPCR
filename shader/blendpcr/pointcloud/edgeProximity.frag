// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#version 330 core

uniform sampler2D rejectedTexture;
uniform int kernelRadius = 10;

in vec2 vScreenPos;

out vec4 FragColor;

void main()
{
    vec2 texelSize = 1.0 / textureSize(rejectedTexture, 0);

    float edgeVal = texture(rejectedTexture, vScreenPos).r;
    if(edgeVal > 0.5){
        FragColor = vec4(1.0, 1.0, 1.0, 1.0);
        return;
    }

	int steps = 3;
    int rad = 5;

    float maxInfluence = 0.0;
    for(float _dX = -rad*steps; _dX <= rad*steps; _dX += steps){
        for(float _dY = -rad*steps; _dY <= rad*steps; _dY += steps){
			float dX = _dX / steps;
			float dY = _dY / steps;
		
            vec2 offset = vec2(dX, dY) * texelSize;
            vec2 coord = vScreenPos + offset;
            float value = texture(rejectedTexture, coord).r;

            if(value > 0.5){
                float d = clamp((rad - sqrt(float(dX) * float(dX) + float(dY) * float(dY))) / rad, 0.0, 1.0);
                if(!isnan(d))
                    maxInfluence = max(maxInfluence, d);
            }
        }
    }

    FragColor = vec4(maxInfluence, maxInfluence, maxInfluence, 1.0);
}
