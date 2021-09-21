#version 430 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoord;

uniform mat4 M;
uniform mat4 V, invV;
uniform mat4 P;

uniform sampler2D displacementTex;
uniform sampler2D normalTex;
uniform int patchCount; // in one dimension

out vec3 world;
out vec3 view;
out vec3 normal;

void main()
{
    vec3 displacedPos = position + texture(displacementTex, texCoord).rgb;
    normal = texture(normalTex, texCoord).rgb;

    int patchCountHalfFloor = (patchCount - 1) / 2;
    vec2 patchShift = (vec2(gl_InstanceID % patchCount, gl_InstanceID / patchCount) - patchCountHalfFloor);
    vec4 shiftedPos = vec4(displacedPos.x + patchShift.x, displacedPos.y, displacedPos.z + patchShift.y, 1.0f);

    vec4 worldPos = M * shiftedPos;
    world = worldPos.xyz;
    vec3 camPos = (invV * vec4(0.0f, 0.0f, 0.0f, 1.0f)).xyz;
    view = normalize(camPos - worldPos.xyz);

    gl_Position = P * V * worldPos;
}