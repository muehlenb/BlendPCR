// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#version 330 core

in vec2 vScreenPos;

uniform sampler2D inputVertices;

uniform int intensity = 3;
uniform float distanceThresholdPerMeter = 0.03f;

layout (location = 0) out vec4 FragPosition;

void main()
{
    int indicator = 0;

    vec3 p = texture(inputVertices, vScreenPos).xyz;
    vec2 texelSize = 1.0 / textureSize(inputVertices, 0);

    FragPosition = vec4(p, 1.0);
    return ;

    for(int dY = -intensity; dY <= intensity; dY += 2){
        for(int dX = -intensity; dX <= intensity; dX += 2){
            float qX = vScreenPos.x + dX * texelSize.x;
            float qY = vScreenPos.y + dY * texelSize.y;

            if(qX < 0 || qX > 1 || qY < 0 || qY > 1)
                continue;

            vec3 q = texture(inputVertices, vec2(qX, qY)).xyz;

            float len = distance(p,q);

            if(isnan(q.x) || len > distanceThresholdPerMeter * p.z)
                --indicator;
            else
                ++indicator;
        }
    }

    if(indicator >= 0){
        FragPosition = vec4(p, 1.0);
    } else {
        FragPosition = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
