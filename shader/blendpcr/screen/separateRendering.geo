#version 330 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in vec4 vPos[];
in vec4 vColor[];
in vec4 vCamPos[];
in float vEdgeDistance[];
in vec2 vCamPosAlpha[];
in vec3 vNormal[];
in vec2 vTexCoord[];

out vec4 fColor;
out vec4 fPos;
out float fEdgeDistance;
out vec2 fPosAlpha;
out vec3 fNormal;
out vec2 fTexCoord;

void main() {
    // Filter all triangles where vertices contains invalid pixels in
    // a-kernel texture:
    for(int i=0; i<3; ++i)
        if(length(vCamPos[i].xyz) < 0.01)
            return;

    // Filter all triangles where vertices contains invalid pixels in
    // edge distance texture:
    for(int i=0; i<3; ++i)
        if(vEdgeDistance[i] > 0.99)
            return;
		
    for(int i=0; i<3; i++)
    {
        gl_Position = gl_in[i].gl_Position;
        fColor = vColor[i];
        fPos = vPos[i];
        fEdgeDistance = vEdgeDistance[i];
        fPosAlpha = vCamPosAlpha[i];
        fNormal = vNormal[i];
        fTexCoord = vTexCoord[i];
        EmitVertex();
    }
	
    EndPrimitive();
}  
