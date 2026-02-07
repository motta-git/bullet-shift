#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D sceneTexture;
uniform sampler2D bloomBlurTexture;
uniform sampler2D depthTexture;

uniform bool bloomEnabled;
uniform float bloomIntensity;
uniform float exposure;

uniform bool fogEnabled;
uniform float fogDensity;
uniform vec3 fogColor;
uniform float nearPlane;
uniform float farPlane;

// Simple color grading parameters
uniform float saturation;
uniform float contrast;
uniform float bulletTimeIntensity;

float linearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
}

void main() {
    vec3 sceneColor = texture(sceneTexture, TexCoords).rgb;
    
    // 1. Fog (Apply before Tonemapping and Bloom to affect the scene)
    if (fogEnabled) {
        float depth = texture(depthTexture, TexCoords).r;
        float linearDepth = linearizeDepth(depth);
        float fogFactor = 1.0 - exp(-fogDensity * linearDepth);
        fogFactor = clamp(fogFactor, 0.0, 1.0);
        sceneColor = mix(sceneColor, fogColor, fogFactor);
    }

    // 2. Bloom
    if (bloomEnabled) {
        vec3 bloomColor = texture(bloomBlurTexture, TexCoords).rgb;
        sceneColor += bloomColor * bloomIntensity;
    }

    // 3. Exposure / Tone mapping
    vec3 result = vec3(1.0) - exp(-sceneColor * exposure);

    // 4. Basic Color Grading
    // Saturation
    float gray = dot(result, vec3(0.299, 0.587, 0.114));
    result = mix(vec3(gray), result, 1.2); // slight saturation boost by default
    
    // 5. Bullet Time Effect
    if (bulletTimeIntensity > 0.0) {
        // Broad desaturation
        float btGray = dot(result, vec3(0.299, 0.587, 0.114));
        result = mix(result, vec3(btGray), bulletTimeIntensity * 0.6);
        
        // Blue tint
        vec3 blueTint = vec3(0.4, 0.7, 1.0);
        result = mix(result, result * blueTint, bulletTimeIntensity * 0.5);
    }
    
    // Contrast
    result = (result - 0.5) * 1.1 + 0.5;

    FragColor = vec4(result, 1.0);
}
