// © 2023, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#version 330 core

in vec2 vScreenPos;

uniform sampler2D vertices;
uniform sampler2D normals;
uniform sampler2D edgeDistances;


out vec2 FragResult;

float easeInOut(float x){
    return 3*x*x + 2*x*x*x;
}


void main()
{		
    float maxCamDist = 6.0;

    vec3 point = texture(vertices, vScreenPos).xyz;
    vec3 normal = texture(normals, vScreenPos).xyz;
    float edgeProximity = texture(edgeDistances, vScreenPos).r;

    float camDist = clamp(length(point), 0.0, maxCamDist);
    float distFactor = (maxCamDist * maxCamDist - camDist * camDist)/36 * clamp(dot(normalize(normal), normalize(point)), 0.1, 1.0);

    FragResult = vec2(distFactor, easeInOut(clamp(1.0-edgeProximity,0.0,1.0)));
}
