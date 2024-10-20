#version 330 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in vec4 vPos[];

out vec4 fColor;
out vec4 fPos;

void main() {    
	float len = max(
		max(
			distance(vPos[0].xyz, vPos[1].xyz),
			distance(vPos[1].xyz, vPos[2].xyz)
		),
		distance(vPos[2].xyz, vPos[0].xyz)
	);
	
	float zDist = max(max(vPos[0].z, vPos[1].z), vPos[2].z);
	
	vec3 normal = normalize(cross(normalize(vPos[2].xyz - vPos[0].xyz), normalize(vPos[1].xyz - vPos[0].xyz)));
	float dotP = dot(normal, vec3(0,0,1));
		
	vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
	
	if(isnan(vPos[0].x) || isnan(vPos[1].x) || isnan(vPos[2].x) || len > 0.026 * zDist)//|| abs(dotP) > 0.5)
		color = vec4(1.0, 1.0, 1.0, 1.0);
	
    for(int i=0; i<3; i++)
    {
		gl_Position = gl_in[i].gl_Position;
		fColor = color;
		fPos = vPos[i];
		EmitVertex();
    }
	
    EndPrimitive();
}  