#version 460 core
out vec4 FragColor;

in float particleDepth;
in vec2 uv;

uniform vec4 particleColor;

void main()
{
    vec2 coord = uv - vec2(0.5);
    float dist = length(coord) * 2.0; // Normalize to 0-1 range
    
    if (dist > 1.0) {
        discard;
    }
    
    float centerGlow = 1.0 - smoothstep(0.0, 0.3, dist);
    float midGlow = 1.0 - smoothstep(0.3, 0.7, dist);
    float outerGlow = 1.0 - smoothstep(0.7, 1.0, dist);
    
    float glow = centerGlow * 1.2 + midGlow * 0.8 + outerGlow * 0.4;
    glow = clamp(glow, 0.0, 1.5);
    
    vec3 finalColor = particleColor.rgb * glow;
    float alpha = particleColor.a * (1.0 - dist * dist);
    float depthFade = clamp(1.0 - (particleDepth / 50.0), 0.0, 1.0);
    alpha *= depthFade;
    alpha = clamp(alpha, 0.0, 1.0);
    
    FragColor = vec4(finalColor, alpha);
}
