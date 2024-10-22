// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#version 330 core

in vec2 vScreenPos;

uniform sampler2D inputVertices;

layout (location = 0) out vec4 FragPosition;

void main()
{
	FragPosition.rgba = texture(inputVertices, vScreenPos).rgba;
}
