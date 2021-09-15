#version 430 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 iNormal;

uniform mat4 M;
uniform mat4 V, invV;
uniform mat4 P;

out vec3 world;
out vec3 view;
out vec3 normal;

void main()
{
    normal = iNormal;
    vec4 worldPos = M * vec4(position, 1.0f);
    world = worldPos.xyz;
    vec3 camPos = (invV * vec4(0.0f, 0.0f, 0.0f, 1.0f)).xyz;
    view = normalize(camPos - worldPos.xyz);

    gl_Position = P * V * worldPos;
}