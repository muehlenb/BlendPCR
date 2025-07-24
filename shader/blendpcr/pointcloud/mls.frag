// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#version 330 core

in vec2 vScreenPos;

uniform float p_h = 1.f;
uniform float kernelSpread = 1.f;
uniform int kernelRadius = 8;

uniform sampler2D pointCloud;
uniform sampler2D edgeProximity;

out vec4 FragColor;

/**
 * Represents the weight function of the implicit surface:
 */
float calculateTheta(vec3 p, vec3 x){
    float d = distance(p, x);
    return exp(-(d*d) / (p_h*p_h));
}

/**
 * Calculates a smoothed vertex for each input vertex / pixel
 */
void main()
{
    // Relative size of one pixel:
    vec2 texelSize = 1.0 / textureSize(pointCloud, 0);

    vec3 mid = texture(pointCloud, vScreenPos).xyz;
	
	if(mid.z < 0.1)
		return;
	
    float edgeDistance = texture(edgeProximity, vScreenPos).r;
	
    if(edgeDistance > 0.99){
        FragColor = vec4(0, 0, -1.0, 1.0);
        return;
    }

    int usedRadius = clamp(int(edgeDistance * 10), 2, kernelRadius);

    vec3 sumPoints = vec3(0,0,0);
    float sumWeights = 0;

    for(int dX = -usedRadius; dX <= usedRadius; ++dX){
        for(int dY = -usedRadius; dY <= usedRadius; ++dY){
            vec2 coord = vScreenPos + vec2(dX, dY) * texelSize;
            vec3 p = texture(pointCloud, coord).xyz;
            float edgeDist = texture(edgeProximity, coord).r;

            if(edgeDist > 0.99)
                continue;

            float theta = 1;
            if(dX != 0 || dY != 0)
                theta = calculateTheta(mid, p);

            float weight = theta * clamp(edgeDist, 0.5, 1.0);
            sumPoints += p * weight;

            sumWeights += clamp(weight,0.000001,100000);
        }
    }

    if(sumWeights > 0.000000001)
        FragColor.rgba = vec4(sumPoints / sumWeights, 1.0);
    else
        FragColor = vec4(0.0, 0.0, 10.0, 1.0);
}
