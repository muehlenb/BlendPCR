// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#version 330 core

in vec2 texCoord;

uniform usampler2D depthTexture;
uniform sampler2D lookupTexture;

out vec4 FragColor;

void main()
{
	ivec2 coords = ivec2(texCoord * vec2(640,576));
	float z = texelFetch(depthTexture, coords, 0).x / 1000.0;
	
	vec2 lookup = texelFetch(lookupTexture, coords, 0).xy;
	
    FragColor = vec4(lookup.x * z, lookup.y * z, z, 1.0);
}
