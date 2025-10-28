#version 330 core

uniform sampler2D texture2D_vertices;
uniform sampler2D texture2D_edgeProximity;
uniform sampler2D texture2D_normals;
uniform sampler2D texture2D_qualityEstimate;

out vec4 vPos;
out vec4 vCamPos;
out float vEdgeDistance;
out vec2 vPosAlpha;
out vec3 vNormal;
out vec2 vTexCoord;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

// LoD-Schrittweite in Texeln (1 = volle Auflösung, 2 = jeder zweite, ...)
uniform int stride;

const ivec2 TRI0[3] = ivec2[3]( ivec2(0,0), ivec2(1,0), ivec2(0,1) );
const ivec2 TRI1[3] = ivec2[3]( ivec2(1,1), ivec2(0,1), ivec2(1,0) );

vec2 uvCenter(ivec2 ij, ivec2 texSize) {
    return (vec2(ij) + vec2(0.5)) / vec2(texSize);
}

struct SampleV {
    vec4 camPos;
    float edge;
    vec3 normal;
    vec2 qual;
};

SampleV sampleAt(ivec2 ij, ivec2 texSize){
    vec2 uv = uvCenter(ij, texSize);
    SampleV s;
    vec4 rawCamPos = texture(texture2D_vertices, uv);
    s.camPos = vec4(rawCamPos.xyz, 1.0);
    s.edge   = texture(texture2D_edgeProximity, uv).r;
    s.normal = texture(texture2D_normals, uv).xyz;
    s.qual   = texture(texture2D_qualityEstimate, uv).rg;
    return s;
}

bool invalidVertex(SampleV s){
    return (s.camPos.z < 0.01) || (s.edge > 0.99);
}

void main() {
    ivec2 texSize = textureSize(texture2D_vertices, 0);
    int cellsX = (texSize.x - 1) / stride; // passt zum C++-Drawcall
    int cellsY = (texSize.y - 1) / stride;

    // coarse cell index aus Instanz
    int cellId = gl_InstanceID;
    int cx = cellId % cellsX;
    int cy = cellId / cellsX;

    ivec2 coarseTL = ivec2(cx * stride, cy * stride);

    // aktuelles Dreieck / Ecke bestimmen
    bool tri1 = (gl_VertexID >= 3);
    int  lid  = tri1 ? (gl_VertexID - 3) : gl_VertexID;

    ivec2 o0 = (tri1 ? TRI1[0] : TRI0[0]) * stride;
    ivec2 o1 = (tri1 ? TRI1[1] : TRI0[1]) * stride;
    ivec2 o2 = (tri1 ? TRI1[2] : TRI0[2]) * stride;

    ivec2 ij0 = coarseTL + o0;
    ivec2 ij1 = coarseTL + o1;
    ivec2 ij2 = coarseTL + o2;

    // Drei Ecken der groben Zelle samplen
    SampleV s0 = sampleAt(ij0, texSize);
    SampleV s1 = sampleAt(ij1, texSize);
    SampleV s2 = sampleAt(ij2, texSize);

    bool triInvalid = invalidVertex(s0) || invalidVertex(s1) || invalidVertex(s2);

    // aktuelles Vertex auswählen
    SampleV sv = (lid==0) ? s0 : (lid==1 ? s1 : s2);

    vCamPos       = sv.camPos;
    vPosAlpha     = sv.qual;
    vEdgeDistance = sv.edge;
    vNormal       = sv.normal;

    ivec2 vOff = (tri1 ? TRI1[lid] : TRI0[lid]) * stride;
    vTexCoord   = uvCenter(coarseTL + vOff, texSize);

    vPos = view * model * vCamPos;

    if (triInvalid) {
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0); // offscreen
    } else {
        gl_Position = projection * vPos;
    }
}

