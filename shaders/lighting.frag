#version 460 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 FragPosLightSpace;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;
    
    float constant;
    float linear;
    float quadratic;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
    
    float constant;
    float linear;
    float quadratic;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

#define NR_POINT_LIGHTS 4

uniform vec3 viewPos;
uniform Material material;
uniform DirLight dirLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight spotLight;
uniform bool u_useHardwareGamma;
uniform float u_time;
uniform float u_techStyleIntensity;

uniform sampler2D shadowMap;

vec3 toLinear(vec3 color) {
    if (u_useHardwareGamma) {
        return pow(max(color, vec3(0.0)), vec3(2.2));
    }
    return color;
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, float shadow);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir);

const vec3 fogColor = vec3(0.0, 0.0, 0.0);
const float fogDensity = 0.015;

vec3 toneMapACES(vec3 color) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

vec3 gammaCorrect(vec3 color) {
    return pow(color, vec3(1.0 / 2.2));
}

vec3 applyTechStyle(vec3 color, vec3 normal, vec3 viewDir, vec3 fragPos) {
    if (u_techStyleIntensity <= 0.0) return color;
    
    // Fresnel for edge detection
    float fresnel = 1.0 - max(dot(viewDir, normal), 0.0);
    fresnel = pow(fresnel, 2.0);
    
    vec3 techColor = vec3(0.05, 0.15, 0.18); // Dark teal-black tech color
    
    // --- Procedural Grid with Derivative-based Anti-Aliasing ---
    
    // Scale the grid coordinates
    float scale = 2.0;
    vec3 pos = fragPos * scale;
    
    // Compute screen-space partial derivatives for anti-aliasing
    // fwidth returns the sum of absolute derivatives in x and y
    vec3 w = fwidth(pos);
    
    // Analytical grid filter
    // 1. fract(pos) creates the repeating gradient 0..1
    // 2. abs(fract(pos) - 0.5) centers it, making lines at integer boundaries
    // 3. Dividing by derivatives (w) normalizes the width in screen pixels
    vec3 grid = abs(fract(pos - 0.5) - 0.5) / (w + 0.0001);
    
    // Calculate line intensity using smoothstep for soft edges dependent on pixel size
    // We want lines to be roughly 1.0 pixel wide regardless of distance
    float lineWidth = 0.8;
    vec3 line = vec3(1.0) - min(grid, 1.0);
    
    // Smooth out the line edge
    // Since we normalized by w, 'grid' is effectively distance in pixels from line center
    // We use a simple 1.0 - saturation(grid / width) approximation usually
    
    // Better approximation for thin lines:
    // Determine visibility of lines on each axis
    float lineX = 1.0 - smoothstep(0.0, lineWidth, grid.x);
    float lineY = 1.0 - smoothstep(0.0, lineWidth, grid.y);
    float lineZ = 1.0 - smoothstep(0.0, lineWidth, grid.z);
    
    // Mix lines based on surface orientation (planar projection)
    float gridPattern = 0.0;
    
    // Blend weights based on normal to avoid stretching artifacts
    vec3 blend = abs(normal);
    blend /= (blend.x + blend.y + blend.z); // Normalize weights
    
    // Project XY grid onto Z-facing surfaces, XZ onto Y, YZ onto X
    gridPattern += blend.z * max(lineX, lineY);
    gridPattern += blend.y * max(lineX, lineZ);
    gridPattern += blend.x * max(lineY, lineZ);
    
    gridPattern = clamp(gridPattern, 0.0, 1.0);
    
    // Fine detail grid (higher frequency)
    vec3 finePos = fragPos * 8.0;
    vec3 fineW = fwidth(finePos);
    vec3 fineGridDist = abs(fract(finePos - 0.5) - 0.5) / (fineW + 0.0001);
    
    float fineLineX = 1.0 - smoothstep(0.0, 1.0, fineGridDist.x);
    float fineLineY = 1.0 - smoothstep(0.0, 1.0, fineGridDist.y);
    float fineLineZ = 1.0 - smoothstep(0.0, 1.0, fineGridDist.z);
    
    float finePattern = 0.0;
    finePattern += blend.z * max(fineLineX, fineLineY);
    finePattern += blend.y * max(fineLineX, fineLineZ);
    finePattern += blend.x * max(fineLineY, fineLineZ);
    
    finePattern = clamp(finePattern, 0.0, 1.0) * 0.3; // Subtler
    
    // Animated scan lines
    float scanLine = sin(fragPos.y * 10.0 + u_time * 3.0) * 0.5 + 0.5;
    scanLine = smoothstep(0.3, 0.7, scanLine) * 0.2;
    
    // Edge glow
    vec3 edgeGlow = techColor * fresnel * 0.6;
    
    // Pulsing effect on grid lines
    float pulse = sin(u_time * 2.5) * 0.5 + 0.5;
    float gridPulse = 0.8 + pulse * 0.2;
    
    // Combine all wireframe effects
    vec3 wireframeColor = techColor * (gridPattern * 0.7 + finePattern) * gridPulse;
    wireframeColor += techColor * scanLine;
    wireframeColor += edgeGlow;
    
    // Darken base color and add wireframe
    float intensity = u_techStyleIntensity;
    color = mix(color, color * 0.4, intensity * 0.6); // Darken base
    color += wireframeColor * intensity;
    
    return color;
}

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    vec3 lightDir = normalize(-dirLight.direction);
    float shadow = ShadowCalculation(FragPosLightSpace, norm, lightDir);
    vec3 result = CalcDirLight(dirLight, norm, viewDir, shadow);
    
    for(int i = 0; i < NR_POINT_LIGHTS; i++)
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);
    
    result += CalcSpotLight(spotLight, norm, FragPos, viewDir);
    
    float ao = 0.2 + 0.8 * max(0.0, dot(norm, vec3(0.0, 1.0, 0.0)));
    result *= ao;
    
    float dist = length(viewPos - FragPos);
    float fogFactor = 1.0 - exp(-fogDensity * dist);
    result = mix(result, fogColor, fogFactor);
    
    result = applyTechStyle(result, norm, viewDir, FragPos);
    
    result = toneMapACES(result);
    
    if (!u_useHardwareGamma) {
        result = gammaCorrect(result);
    }
    
    FragColor = vec4(result, 1.0);
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, float shadow)
{
    vec3 lightDir = normalize(-light.direction);
    
    float diff = max(dot(normal, lightDir), 0.0);
    
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess * 2.0);
    
    float rim = 1.0 - max(dot(viewDir, normal), 0.0);
    rim = smoothstep(0.6, 1.0, rim);
    vec3 rimColor = vec3(0.1) * rim;
    
    vec3 ambient  = toLinear(light.ambient)  * toLinear(material.ambient);
    vec3 diffuse  = toLinear(light.diffuse)  * diff * toLinear(material.diffuse);
    vec3 specular = toLinear(light.specular) * spec * toLinear(material.specular);
    
    return (ambient + (1.0 - shadow) * (diffuse + specular) + rimColor);
}

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    
    // check whether current frag pos is in shadow
    // bias to prevent shadow acne
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);
    
    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
        shadow = 0.0;
        
    return shadow;
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    
    float diff = max(dot(normal, lightDir), 0.0);
    
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess * 2.0);
    
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    
    vec3 ambient  = toLinear(light.ambient)  * toLinear(material.ambient);
    vec3 diffuse  = toLinear(light.diffuse)  * diff * toLinear(material.diffuse);
    vec3 specular = toLinear(light.specular) * spec * toLinear(material.specular);
    
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;
    
    return (ambient + diffuse + specular);
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    
    float diff = max(dot(normal, lightDir), 0.0);
    
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess * 2.0);
    
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    intensity = smoothstep(0.0, 1.0, intensity);
    
    vec3 ambient  = toLinear(light.ambient)  * toLinear(material.ambient);
    vec3 diffuse  = toLinear(light.diffuse)  * diff * toLinear(material.diffuse);
    vec3 specular = toLinear(light.specular) * spec * toLinear(material.specular);
    
    ambient  *= attenuation * intensity;
    diffuse  *= attenuation * intensity;
    specular *= attenuation * intensity;
    
    return (ambient + diffuse + specular);
}
