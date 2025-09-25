#version 410 core

// Input vertex attributes - set in object constructors
layout (location = 0) in vec3 position;  // Vertex position in local space (model space)
layout (location = 1) in vec2 texcoord;  // texture coordinate of vertex
layout (location = 2) in vec3 normal;    // normals of the vertex

//for normal map
layout (location = 3) in vec3 tangent;    // Tangent of the vertex
layout (location = 4) in vec3 bitangent;  // Bitangent of the vertex

// Output to fragment shader
out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

//for normal map
out vec3 Tangent;
out vec3 Bitangent;

// Uniforms for transformation matrices
uniform mat4 u_model;       // Model matrix: transforms from local space to world space
uniform mat4 u_view;        // View matrix: transforms from world space to camera space (view space)
uniform mat4 u_projection;  // Projection matrix: transforms from camera space to clip space

void main()
{
    // Apply the transformations to the vertex position
    gl_Position = u_projection * u_view * u_model * vec4(position, 1.0);
    
    FragPos = vec3(u_model * vec4(position,1.0));
    Normal = mat3(transpose(inverse(u_model))) * normal;
    TexCoords = texcoord;
    Tangent = mat3(u_model) * tangent;
    Bitangent = mat3(u_model) * bitangent;
}