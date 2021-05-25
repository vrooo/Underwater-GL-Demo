#version 330 core
out vec4 oColor;

uniform vec3 color;

in vec3 world;
in vec3 view;
in vec3 normal;

void main()
{
    // TODO: uniforms
    vec3 lightPos = vec3(0, 10, 0);
    vec3 ambientCol = vec3(0.5);
    vec3 lightCol = vec3(1);
    float kd = 0.5, ks = 0.5, m = 10;

    vec3 col = color * ambientCol;

    vec3 light = normalize(lightPos - world);
    col += color * lightCol * kd * clamp(dot(normal, light), 0, 1);

    vec3 halfVec = normalize(view + light);
    float nh = clamp(pow(dot(normal, halfVec), m), 0, 1);
    col += lightCol * ks * nh;

    oColor = vec4(col, 1);
}