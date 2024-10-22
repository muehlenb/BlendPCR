#version 330 core

in vec4 fColor;
in vec4 fPos;
in vec3 fNormal;
in float fEdgeDistance;
in vec2 fPosAlpha;
in vec2 fTexCoord;

uniform sampler2D texture2D_colors;
uniform sampler2D highResTexture;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform bool useColorIndices = false;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 FragPosition;
layout (location = 2) out vec4 FragNormal;

int mode(int values[9]) {
    int maxValue = values[0];
    int maxCount = 1;

    for (int i = 0; i < 9; i++) {
        int count = 1;
        for (int j = 0; j < 9; j++) {
            if (i != j && values[i] == values[j]) {
                count++;
            }
        }
        if (count > maxCount) {
            maxCount = count;
            maxValue = values[i];
        }
    }

    return maxValue;
}

vec2 decodeTexCoords(float r, float g, int b){
    if(b % 2 == 1)
        r = 255.0 - r;

    if(int(b / 32) % 2 == 1)
        g = 255.0 - g;

    float x = r/4.0 + (int(b) % 32) * 64;
    float y = g/4.0 + int(b / 32) * 64;

    return vec2(x / 2048.0, y / 1536.0);
}

void main()
{
    if(useColorIndices){
        vec2 tex0 = texture(texture2D_colors, fTexCoord - vec2(0.0, 0.0028)).ra;
        vec2 tex1 = texture(texture2D_colors, fTexCoord - vec2(0.0, 0.0021)).ra;
        vec2 tex2 = texture(texture2D_colors, fTexCoord - vec2(0.0, 0.0014)).ra;
        vec2 tex3 = texture(texture2D_colors, fTexCoord - vec2(0.0, 0.0007)).ra;
        vec2 tex4 = texture(texture2D_colors, fTexCoord).ra;
        vec2 tex5 = texture(texture2D_colors, fTexCoord + vec2(0.0, 0.0007)).ra;
        vec2 tex6 = texture(texture2D_colors, fTexCoord + vec2(0.0, 0.0014)).ra;
        vec2 tex7 = texture(texture2D_colors, fTexCoord + vec2(0.0, 0.0021)).ra;
        vec2 tex8 = texture(texture2D_colors, fTexCoord + vec2(0.0, 0.0028)).ra;

        int bValues[9];
        bValues[0] = int((tex0.x * 256)) + (int((tex0.y * 256)) << 8);
        bValues[1] = int((tex1.x * 256)) + (int((tex1.y * 256)) << 8);
        bValues[2] = int((tex2.x * 256)) + (int((tex2.y * 256)) << 8);
        bValues[3] = int((tex3.x * 256)) + (int((tex3.y * 256)) << 8);
        bValues[4] = int((tex4.x * 256)) + (int((tex4.y * 256)) << 8);
        bValues[5] = int((tex5.x * 256)) + (int((tex5.y * 256)) << 8);
        bValues[6] = int((tex6.x * 256)) + (int((tex6.y * 256)) << 8);
        bValues[7] = int((tex7.x * 256)) + (int((tex7.y * 256)) << 8);
        bValues[8] = int((tex8.x * 256)) + (int((tex8.y * 256)) << 8);

        int b = mode(bValues);

        vec4 texCoord = texture(texture2D_colors, fTexCoord).rgba;

        FragColor = texture(highResTexture, decodeTexCoords(texCoord.b * 256, texCoord.g * 256, b)).bgra;
    } else {
        FragColor = fColor.bgra;
    }

    FragNormal = vec4(fNormal, fPosAlpha.y);
    FragPosition = vec4(fPos.xyz, fPosAlpha.x);
}
