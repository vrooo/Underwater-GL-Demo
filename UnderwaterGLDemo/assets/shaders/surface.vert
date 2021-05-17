#version 330 core
layout (location = 0) in vec3 positionToRemove;
layout (location = 1) in ivec2 texCoord;

//uniform mat4 M;
uniform mat4 V, invV;
uniform mat4 P;

uniform sampler2D waveTex;

out vec3 world;
out vec3 view;
out vec3 normal;

void main()
{
    world = texelFetch(waveTex, texCoord, 0).xyz;
    vec4 worldPos = vec4(world, 1.0f);
    vec3 camPos = (invV * vec4(0.0f, 0.0f, 0.0f, 1.0f)).xyz;
    view = normalize(camPos - worldPos.xyz);

    // TODO: dependent on scene size
    vec3 t1 = normalize(vec3(0.8f, texelFetch(waveTex, texCoord - ivec2(1, 0), 0).y - texelFetch(waveTex, texCoord + ivec2(1, 0), 0).y, 0.f));
    vec3 t2 = normalize(vec3(0.f, texelFetch(waveTex, texCoord - ivec2(0, 1), 0).y - texelFetch(waveTex, texCoord + ivec2(0, 1), 0).y, 0.8f));
    normal = cross(t2, t1);
    gl_Position = P * V * worldPos;
}