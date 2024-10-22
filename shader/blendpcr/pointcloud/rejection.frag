#version 330 core

in vec2 texCoord;

uniform sampler2D pointCloud;
uniform sampler2D colorTexture;
uniform mat4 model;

out vec4 FragColor;

void main()
{
    vec2 texelSize = 1.0 / textureSize(pointCloud, 0);
    vec2 halfTexelSize = 0.5 / textureSize(pointCloud, 0);

    vec3 point = texture(pointCloud, texCoord).xyz;
    vec3 rgb = texture(colorTexture, texCoord).xyz;

    if(rgb.r < 0.01 && rgb.g < 0.01 && rgb.b < 0.01){
        FragColor = vec4(1.0, 1.0, 1.0, 1.0);
        return;
    }

    //if(isnan(point.x) || isnan(point.y) || isnan(point.z)){
    //	FragColor = vec4(1.0, 1.0, 1.0, 1.0);
    //	return;
    //}

    // Background Removal via cylinder in the mid of the scene:
    //vec3 modelPoint = (model * vec4(point, 1.0)).xyz;
    //if(length(modelPoint.xz) > 0.87 || modelPoint.y < 0.05 || modelPoint.y > 1.95){
    //	FragColor = vec4(1.0, 1.0, 1.0, 1.0);
    //	return;
    //}

    vec4 color = vec4(0.0, 0.0, 0.0, 1.0);

    for(int x = -1; x <= 1; ++x){
        for(int y = -1; y <= 1; ++y){
            if(x == 0 && y == 0)
                continue;

            vec2 otherTexCoord = texCoord + vec2(x, y) * texelSize;
            vec3 otherPoint = texture(pointCloud, otherTexCoord).xyz;

            if(isnan(otherPoint.x) || isnan(otherPoint.y) || isnan(otherPoint.z)){
                FragColor = vec4(1.0, 1.0, 1.0, 1.0);
                return;
            }

            float len = length(otherPoint - point);


            if(len > 0.015 * point.z || isnan(len)){
                FragColor = vec4(1.0, 1.0, 1.0, 1.0);
                return;
            }
        }
    }

    FragColor = vec4(0.0, 0.0, 0.0, 1.0);
}
