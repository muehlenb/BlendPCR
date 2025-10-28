// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#version 330 core

in vec2 vScreenPos;

uniform sampler2D currentVertices;
uniform sampler2D previousVertices;

uniform float smoothing = 0.99;

layout (location = 0) out vec4 FragPosition;

void main()
{	
	vec4 current = texture(currentVertices, vScreenPos);
	FragPosition = current;
}
