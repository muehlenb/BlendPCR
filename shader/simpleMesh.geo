#version 330 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

uniform float maxEdgeLength = 0.1;
uniform bool discardBlackPixels = false;

in vec4 vWorldPosition[];
in vec4 vColor[];

out vec4 fWorldPosition;
out vec4 fColor;

void main() {
	// If World position is 0.0 or invalid, return:
	for(int i = 0; i < 3; ++i){
		float len = length(vWorldPosition[i].xyz);
		
		// Includes check for NAN by negation:
		if(!(len > 0.01) || (length(vColor[i].xyz) == 0 && discardBlackPixels))
			return;
    }
	
	for(int i = 0; i < 3; ++i){
		float dist = length(vWorldPosition[i].xyz - vWorldPosition[(i+1)%3].xyz);
		
		if(dist > maxEdgeLength)
			return;
	}

    for(int i = 0; i < 3; ++i)
    {
		gl_Position = gl_in[i].gl_Position;
		fColor = vColor[i];
		fWorldPosition = vWorldPosition[i];
		EmitVertex();
    }
	
    EndPrimitive();
}  