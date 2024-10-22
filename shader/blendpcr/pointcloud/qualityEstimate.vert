#version 330 core

layout (location = 0) in vec2 vInPos;   // the position attribute

out vec2 vScreenPos;

void main()
{
	vScreenPos = vInPos.xy * 0.5 + 0.5;
    gl_Position = vec4(vInPos.xy, 0.5, 1.0);

}