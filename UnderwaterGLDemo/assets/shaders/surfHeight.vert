#version 430 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoord;

//uniform mat4 M;
uniform mat4 V, invV;
uniform mat4 P;

uniform sampler2D normalHeightTex;

out vec3 world;
out vec3 view;
out vec3 normal;

void main()
{
    //normal = vec3(0.0f, 1.0f, 0.0f); // TODO

    vec4 normalHeight = texture(normalHeightTex, texCoord);
    normal = normalHeight.rgb;
    world = vec3(position.x, normalHeight.a, position.z);
    vec4 worldPos = vec4(world, 1.0f);
    vec3 camPos = (invV * vec4(0.0f, 0.0f, 0.0f, 1.0f)).xyz;
    view = normalize(camPos - worldPos.xyz);

    gl_Position = P * V * worldPos;
}