#version 460 core
layout (location = 0) in vec3 aPos;

out float particleDepth;
out vec2 uv;

uniform mat4 projection;
uniform mat4 view;
uniform vec3 particlePos;
uniform float particleSize;

void main()
{
    // Billboard effect - particle always faces camera
    vec3 cameraRight = vec3(view[0][0], view[1][0], view[2][0]);
    vec3 cameraUp = vec3(view[0][1], view[1][1], view[2][1]);
    
    // Scale based on distance for depth perception
    vec4 viewPos = view * vec4(particlePos, 1.0);
    float depth = -viewPos.z;
    particleDepth = depth;
    
    // Slight size scaling based on distance
    float sizeScale = 1.0 + (depth * 0.01);
    
    vec3 vertexPos = particlePos 
                   + cameraRight * aPos.x * particleSize * sizeScale
                   + cameraUp * aPos.y * particleSize * sizeScale;

    uv = aPos.xy + vec2(0.5); // map quad corners [-0.5,0.5] to [0,1]
    
    gl_Position = projection * view * vec4(vertexPos, 1.0);
}
