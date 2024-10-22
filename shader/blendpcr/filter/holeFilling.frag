// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#version 330 core

in vec2 vScreenPos;

uniform sampler2D inputVertices;
uniform sampler2D inputColors;

layout (location = 0) out vec4 FragPosition;
layout (location = 1) out vec4 FragColor;

void main()
{
	FragPosition.rgba = texture(inputVertices, vScreenPos).rgba;
	FragColor.rgba = texture(inputColors, vScreenPos).rgba;
}
