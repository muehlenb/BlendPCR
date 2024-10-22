// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#version 330 core

in vec2 vScreenPos;

uniform sampler2D inputVertices;
uniform sampler2D inputColors;
uniform sampler2D lookupImageTo3D;

uniform float requiredValidNeighborRatio = 0.1f;
uniform int intensity = 3;
uniform float maxDistance = 0.1f;

layout (location = 0) out vec4 FragPosition;
layout (location = 1) out vec4 FragColor;

void main()
{
    vec2 texelSize = 1.0 / textureSize(inputVertices, 0);
    vec3 p = texture(inputVertices, vScreenPos).rgb;
    vec4 pCol = texture(inputColors, vScreenPos).rgba;

    // If point is valid, we don't need to fill it:
    if(!isnan(p.x) && p.z >= 0.01f){
        FragPosition = vec4(p, 1.0);
        FragColor = pCol + vec4(0.0, 0.5, 0.0, 0.0);
        return;
    }

    float sumDepth = 0.f;
    vec3 sumCol = vec3(0, 0, 0);
    float sumWeight = 0;

    int validNeighbors = 0;
    int totalNeighbors = 0;

    for(int dY = -intensity; dY <= intensity; ++dY){
        for(int dX = -intensity; dX <= intensity; ++dX){
            if(abs(dX) + abs(dY) > intensity)
                continue;

            float qX = vScreenPos.x + dX * texelSize.x;
            float qY = vScreenPos.y + dY * texelSize.y;

            if(qX < 0 || qX > 1 || qY < 0 || qY > 1)
                continue;

            vec3 q = texture(inputVertices, vec2(qX, qY)).xyz;
            vec3 qCol = texture(inputColors, vec2(qX, qY)).rgb;

            if(!isnan(q.x) && q.z >= 0.01f){
                float weight = 1.f;

                sumDepth += length(q) * weight;
                sumWeight += weight;

                sumCol += qCol * weight;

                ++validNeighbors;
            }
            ++totalNeighbors;
        }
    }

    vec2 xyPart = texture(lookupImageTo3D, vScreenPos).rg;
    if(validNeighbors / float(totalNeighbors) >= requiredValidNeighborRatio){
        float repairedLength = sumDepth / sumWeight;
        float repairedDepth = repairedLength / sqrt(xyPart.x*xyPart.x + xyPart.y*xyPart.y + 1);

        FragPosition = vec4(xyPart.x * repairedDepth, xyPart.y * repairedDepth, repairedDepth, 1.0);
        FragColor = vec4(1.0, 0.0, 1.0, 1.0); //vec4(sumCol / sumWeight, 1.0);
        return;
    }

    FragPosition = vec4(p, 1.0);
    FragColor = vec4(1.0, 0.0, 1.0, 1.0);
}
