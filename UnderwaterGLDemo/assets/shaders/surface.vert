#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in ivec2 texCoord;

//uniform mat4 M;
uniform mat4 V, invV;
uniform mat4 P;

uniform sampler2D waveTex;
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
        vec4 wave1 = texelFetch(waveTex, ivec2(i, 0), 0); // k.x, k.y, amplitude, omega
        vec4 wave2 = texelFetch(waveTex, ivec2(i, 1), 0); // phase shift, 0, 0, 0
        float amp = wave1.z, omega = wave1.w, phShift = wave2.x;
        vec2 kVec = wave1.xy;
        vec2 kNorm = normalize(kVec);
        float phase = kVec.x * position.x + kVec.y * position.z - omega * t + phShift;
        float sinp = sin(phase), cosp = cos(phase);
        vec2 disp = kNorm * amp * sinp;

        accX += disp.x;
        accY += amp * cosp;
        accZ += disp.y;

        vec2 normalDisp = kNorm * amp * omega * cosp;
        normal.x += normalDisp.x;
        normal.y += amp * omega * sinp;
        normal.z += normalDisp.y;
    }

    world = vec3(position.x - accX, accY, position.z - accZ);
    vec4 worldPos = vec4(world, 1.0f);
    vec3 camPos = (invV * vec4(0.0f, 0.0f, 0.0f, 1.0f)).xyz;
    view = normalize(camPos - worldPos.xyz);

    gl_Position = P * V * worldPos;
}