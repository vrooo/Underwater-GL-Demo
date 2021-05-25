#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in ivec2 texCoord;

//uniform mat4 M;
uniform mat4 V, invV;
uniform mat4 P;

uniform sampler1D waveTex;
uniform float waveCount;
uniform float t;

out vec3 world;
out vec3 view;
out vec3 normal;

void main()
{
    float accX = 0.0f, accY = 0.0f, accZ = 0.0f;
    normal = vec3(0.0f, 1.0f, 0.0f);
    // TODO: more like GPU Gems?
    for (int i = 0; i < waveCount; i++)
    {
        vec4 wave = texelFetch(waveTex, i, 0); // k.x, k.y, amplitude, omega
        vec2 kNorm = normalize(wave.xy);
        float phase = wave.x * position.x + wave.y * position.z - wave.w * t;
        float sinp = sin(phase), cosp = cos(phase);
        vec2 disp = kNorm * wave.z * sinp;

        accX += disp.x;
        accY += wave.z * cosp;
        accZ += disp.y;

        vec2 normalDisp = kNorm * wave.z * wave.w * cosp;
        normal.x += normalDisp.x;
        normal.y += wave.z * wave.w * sinp;
        normal.z += normalDisp.y;
    }

    world = vec3(position.x - accX, accY, position.z - accZ);
    vec4 worldPos = vec4(world, 1.0f);
    vec3 camPos = (invV * vec4(0.0f, 0.0f, 0.0f, 1.0f)).xyz;
    view = normalize(camPos - worldPos.xyz);

    // TODO: dependent on scene size
    //vec3 t1 = normalize(vec3(0.8f, texelFetch(waveTex, texCoord - ivec2(1, 0), 0).y - texelFetch(waveTex, texCoord + ivec2(1, 0), 0).y, 0.f));
    //vec3 t2 = normalize(vec3(0.f, texelFetch(waveTex, texCoord - ivec2(0, 1), 0).y - texelFetch(waveTex, texCoord + ivec2(0, 1), 0).y, 0.8f));
    //normal = cross(t2, t1);
    //normal = vec3(0, 1, 0);
    gl_Position = P * V * worldPos;
}