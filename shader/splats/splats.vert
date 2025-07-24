#version 330

/** Color from vertex shader */
out vec4 color_from_vs;

/** World position */
out vec4 world_position;

/** Position texture */
uniform usampler2D depthTexture;

/** Color texture */
uniform sampler2D colorTexture;

/** Lookup texture */
uniform sampler2D lookupTexture;
/**
 * Projection matrix which performs a perspective transformation.
 */
uniform mat4 projection;

/**
 * View matrix which transforms the vertices from world space
 * into camera space.
 */
uniform mat4 view;

/**
 * Model matrix which transforms the vertices from model space
 * into world space.
 */
uniform mat4 model;

/**
 * Since this is the vertex shader, the following program is
 * executed for each vertex (i.e. triangle point) individually.
 */
void main()
{
	ivec2 texSize = textureSize(depthTexture, 0); 
	
	float u = float(gl_VertexID % texSize.x + 0.5) / texSize.x;
	float v = float(gl_VertexID / texSize.x + 0.5) / texSize.y;
	
	float depth = float(texture(depthTexture, vec2(u, v)).x) / 1000.0;
	vec2 lookup = texture(lookupTexture, vec2(u, v)).xy;
	
	vec4 position = vec4(lookup.x * depth, lookup.y * depth , depth, 1.0);
	
	vec4 color = texture(colorTexture, vec2(u, v));
	
	color_from_vs = color;
	world_position = model * position;

	// Set point size:
	gl_PointSize = 3;
	
    // Calculate gl_Position:
    gl_Position = projection * view * world_position;
}
