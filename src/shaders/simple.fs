#version 410 core

const int MAX_LIGHTS = 16;

out vec4 FragColor;  // Output color of the fragment (pixel)

//variables passed in by vertex shader
in vec3 FragPos; // worldspace position of the fragment (pixel)
in vec3 Normal; // Normal map of the fragment for diffuse lighting
in vec2 TexCoords; // Texture coordinate of the fragment (pixel)
//for normal map
in vec3 Tangent;
in vec3 Bitangent;

//variables passed in by our object
uniform vec3 meshColor = vec3(1.0, 1.0, 1.0); // model albedo
uniform bool useMeshTexture = false; // use texture
uniform sampler2D u_texture; //texture reference index

uniform float roughness = 0.5;
uniform bool useRoughnessMap = false;
uniform sampler2D roughnessMap;

uniform float metalness = 0.0;
uniform bool useMetalnessMap = false;
uniform sampler2D metalnessMap;

uniform bool useNormalMap = false;
uniform sampler2D normalMap;

//variables passed in main loop
uniform vec3 viewPos; //viewport position
uniform int numPointLights; //number of point lights in scene
uniform int numSpotLights; //number of spot lights in scene
uniform vec3 globalAmbient; // Global ambient light color

// Shadow mapping
uniform sampler2D shadowMap;
uniform mat4 lightSpaceMatrix;

// Shadow calculation function
float ShadowCalculation(vec3 fragPos)
{
    // Transform fragment position to light space
    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(fragPos, 1.0);
    // Perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // Check if outside shadow map
    if(projCoords.z > 1.0) return 0.0;
    // Get closest depth from shadow map
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    // Bias to prevent shadow acne
    float bias = 0.0002;
    // Simple shadow test
    float shadow = currentDepth > closestDepth + bias ? 1.0 : 0.0;
    //shadow = 1;
    //FragColor = vec4(vec3(texture(shadowMap, projCoords.xy).r), 1.0);
    return shadow;
}


struct PointLight {
    vec3 position;
    vec3 color;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    vec3 color;
    float cutoff;
};

uniform PointLight lights[MAX_LIGHTS];
uniform SpotLight spotLights[MAX_LIGHTS];

vec3 calcLight(vec3 lightPos, vec3 lightColor, vec3 norm, vec3 viewDir, vec3 albedo, float rough, float metal, float shadow, vec3 fragPos, bool isSpot, vec3 spotDir, float cutoff) {
    float distance = length(lightPos - fragPos);
    float attenuation = 1.0 / (1.0 + 0.12 * distance + 0.05 * (distance * distance));
    float intensity = (lightColor.x + lightColor.y + lightColor.z) / 3.0;
    if (attenuation * intensity * 0.5 < 0.03) return vec3(0);
    float ambientStrength = 0.5;
    vec3 ambient = ambientStrength * lightColor;
    vec3 lightDir = normalize(lightPos - fragPos);
    float spotEffect = 1.0;
    if (isSpot) {
        float theta = dot(normalize(-lightDir), normalize(spotDir));
        float epsilon = 0.01;
        spotEffect = smoothstep(cutoff - epsilon, cutoff + epsilon, theta);
        if (theta < cutoff) return vec3(0);
    }
    float diff = max(dot(norm, lightDir), 0.0) * spotEffect;
    vec3 diffuse = diff * lightColor;
    float specularStrength = (1.0 - rough);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor * (0.5 + metal * 0.5) * spotEffect;
    diffuse *= (1.0 - metal);
    specular *= (0.5 + metal * 0.5);
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return ambient * albedo + (1.0 - shadow) * (diffuse * albedo + specular);
}

vec3 getAlbedo() {
    if (useMeshTexture) {
        return texture(u_texture, TexCoords).rgb * meshColor;
    } else {
        return meshColor;
    }
}

float getRoughness() {
    if (useRoughnessMap) {
        return texture(roughnessMap, TexCoords).r;
    } else {
        return roughness;
    }
}

float getMetalness() {
    if (useMetalnessMap) {
        return texture(metalnessMap, TexCoords).r;
    } else {
        return metalness;
    }
}

void main() {
    vec3 norm = normalize(Normal);

    // Normal mapping
    if (useNormalMap) {
        vec3 T = normalize(Tangent);
        vec3 B = normalize(Bitangent);
        vec3 N = norm;
        mat3 TBN = mat3(T, B, N);

        vec3 normalMap = texture(normalMap, TexCoords).rgb;
        normalMap = normalMap * 2.0 - 1.0; // Transform from [0,1] to [-1,1]
        norm = normalize(TBN * normalMap);
    }

    float rough = clamp(getRoughness(), 0.05, 1.0); // avoid 0 roughness
    float metal = clamp(getMetalness(), 0.0, 1.0);
    vec3 albedo = getAlbedo();

    vec3 viewDir = normalize(viewPos - FragPos);

    // Calculate shadow
    float shadow = ShadowCalculation(FragPos);

    vec3 result = globalAmbient * albedo;

    // Point lights
    for (int i = 0; i < numPointLights; ++i) {
        result += calcLight(lights[i].position, lights[i].color, norm, viewDir, albedo, rough, metal, shadow, FragPos, false, vec3(0), 0.0);
    }
    // Spot lights
    for (int i = 0; i < numSpotLights; ++i) {
        result += calcLight(spotLights[i].position, spotLights[i].color, norm, viewDir, albedo, rough, metal, shadow, FragPos, true, spotLights[i].direction, spotLights[i].cutoff);
    }
    FragColor = vec4(result, useMeshTexture ? texture(u_texture, TexCoords).a : 1.0);

}