#version 430 core
out vec4 oColor;

uniform vec3 ambientColor;
uniform vec3 diffuseColor;
uniform vec3 specularColor;
uniform float specularHighlight;

in vec3 world;
in vec3 view;
in vec3 normal;

void main()
{
    // TODO: light uniforms
    vec3 lightPos = vec3(0, 10, 0);
    vec3 lightCol = vec3(1);
    float ambientLightLevel = 0.3f;

    vec3 col = ambientLightLevel * ambientColor;

    vec3 light = normalize(lightPos - world);
    col += ambientColor * diffuseColor * lightCol * clamp(dot(normal, light), 0, 1);

    vec3 halfVec = normalize(view + light);
    float nh = clamp(pow(dot(normal, halfVec), specularHighlight), 0, 1);
    col += specularColor * lightCol * nh;

    oColor = vec4(col, 1);
}