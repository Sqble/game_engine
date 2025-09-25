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
uniform int numLights; //number of lights in scene
uniform vec3 globalAmbient; // Global ambient light color

struct PointLight {
    vec3 position;
    vec3 color;
};

uniform PointLight lights[MAX_LIGHTS];

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

void main()
{
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

    vec3 result = globalAmbient * albedo;

    for (int i = 0; i < numLights; ++i) {
        // calculate distance falloff of light point
        float distance = length(lights[i].position - FragPos);
        float attenuation = 1.0 / (1.0 + 0.12 * distance + 0.05 * (distance * distance));

        //skip light if too far away
        float intensity = (lights[i].color.x + lights[i].color.y + lights[i].color.z) / 3.0;
        if (attenuation * intensity * 0.5 < 0.03) {
            continue;
        }

        //ambient
        float ambientStrength = 0.5;
        vec3 ambient = ambientStrength * lights[i].color;

        //diffuse
        vec3 lightDir = normalize(lights[i].position - FragPos);
        float diff = max(dot(norm,lightDir), 0.0);
        vec3 diffuse = diff * lights[i].color;

        //specular
        float specularStrength = (1.0 - rough); // more rough = less specular
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32); // Shininess factor hardcoded to 32
        //vec3 specular = specularStrength * spec * lights[i].color;
        vec3 specular = specularStrength * spec * lights[i].color * (0.5 + metal * 0.5);

        // more metallic = less diffuse, more specular
        diffuse *= (1.0 - metalness);
        specular *= (0.5 + metalness * 0.5);

        // apply distance falloff to lighting components
        ambient *= attenuation;
        diffuse *= attenuation;
        specular *= attenuation;

        result += (ambient + diffuse) * albedo + specular;
    }

    FragColor = vec4(result, useMeshTexture ? texture(u_texture, TexCoords).a : 1.0);

}