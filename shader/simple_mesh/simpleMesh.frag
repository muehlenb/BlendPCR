#version 330

in vec4 fWorldPosition;
in vec4 fColor;

/**
 * This first defined out variable of the fragment shader defines
 * with which color the fragment is rendered
 */
out vec4 fragment_color;


void main()
{
    fragment_color = fColor.bgra;
}
