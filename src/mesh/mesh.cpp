#include "mesh.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <gl/glew.h>

#include <map>
#include <tuple>
#include <algorithm>

#include "../objmanager/objmanager.h"

void Mesh::updateVboVertexData() {
    bufferData.clear();
    // Prepare bufferData: position (3), texcoord (2)
    for (size_t i = 0; i < vertices.size(); ++i) {
        // Apply position offset
        bufferData.push_back(vertices[i].x);
        bufferData.push_back(vertices[i].y);
        bufferData.push_back(vertices[i].z);

        // Normal (placeholder)
        bufferData.push_back(normals[i].x);
        bufferData.push_back(normals[i].y);
        bufferData.push_back(normals[i].z);

        // Texcoord
        bufferData.push_back(texcoords[i].x);
        bufferData.push_back(texcoords[i].y);

        // Tangent and Bitangent (if normal map is used)
        if (material.hasNormalMap()) {
            /*
            if (i >= tangents.size() || i >= bitangents.size()) {
                std::cerr << "Tangent/bitangent index out of bounds: " << i << std::endl;
                continue; // or break, or throw
            }*/

            bufferData.push_back(tangents[i].x);
            bufferData.push_back(tangents[i].y);
            bufferData.push_back(tangents[i].z);

            bufferData.push_back(bitangents[i].x);
            bufferData.push_back(bitangents[i].y);
            bufferData.push_back(bitangents[i].z);
        }
    }
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, bufferData.size() * sizeof(float), bufferData.data(), GL_STATIC_DRAW);
}

Mesh::Mesh(const std::string& objPath,
         const Material& mat,
         const glm::vec3& position,
         const glm::vec3& size,
         const glm::vec3& color,
         const glm::vec3& rotation,
         bool isActive)
         : DrawableObject(position, glm::vec3(1), color, rotation, isActive), material(mat) {
    this->objPath = objPath;

    OBJData objData = OBJManager::getOBJ(objPath, material.getAlbedoPath(), material.getMetalPath(), material.getRoughPath(), material.getNormalPath());
    vertices = std::move(objData.vertices);
    texcoords = std::move(objData.texcoords);
    normals = std::move(objData.normals);
    indices = std::move(objData.indices);

    // Tangent and Bitangent (if normal map is used)
    if (material.hasNormalMap()) {
        tangents = std::move(objData.tangents);
        bitangents = std::move(objData.bitangents);
    }

    glBindVertexArray(VAO);

    updateVboVertexData();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    int stride = material.hasNormalMap() ? (14 * sizeof(float)) : (8 * sizeof(float));

    // Position attribute (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    // Texcoord attribute (location = 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Normal attribute (location = 2)
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);

    if (material.hasNormalMap()) {
        // Tangent attribute (location = 3)
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(3);

        // Bitangent attribute (location = 4)
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride, (void*)(11 * sizeof(float)));
        glEnableVertexAttribArray(4);
    }

    glBindVertexArray(0);
    setSize(size);
    setRotation(rotation);
    computeBoundingBox();
}

void Mesh::draw(Shader& shader) {
    if (!isActive_) return;

    if (material.hasAlbedoMap()) {
        material.getAlbedoMap()->bind(0);
        shader.setInt("u_texture",0);
        shader.setBool("useMeshTexture", true);
    }
    else {
        shader.setBool("useMeshTexture", false);
    }

    if (material.hasRoughnessMap()) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, material.getRoughnessMap()->getID());
        shader.setInt("roughnessMap", 1);
        shader.setBool("useRoughnessMap", true);
    } else {
        shader.setBool("useRoughnessMap", false);
    }

    if (material.hasMetalnessMap()) {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, material.getMetalnessMap()->getID());
        shader.setInt("metalnessMap", 2);
        shader.setBool("useMetalnessMap", true);
    } else {
        shader.setBool("useMetalnessMap", false);
    }

    if (material.hasNormalMap()) {
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, material.getNormalMap()->getID());
        shader.setInt("normalMap", 3);
        shader.setBool("useNormalMap", true);
        //std::cout << "Using normal map for mesh: " << material.getNormalPath() << std::endl;
    } else {
        shader.setBool("useNormalMap", false);
    }

    shader.setMat4("u_model", model_);
    shader.setVec3("meshColor", color_);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    
    // Clean up shader state to avoid affecting other objects
    shader.setBool("useMeshTexture", false);
    shader.setBool("useRoughnessMap", false);
    shader.setBool("useMetalnessMap", false);
}
/*
void Mesh::setSize(const glm::vec3& size) {
    // Compute scale factor based on the ratio of new size to current size
    glm::vec3 scaleFactor;
    scaleFactor.x = (size_.x != 0.0f) ? (size.x / size_.x) : 1.0f;
    scaleFactor.y = (size_.y != 0.0f) ? (size.y / size_.y) : 1.0f;
    scaleFactor.z = (size_.z != 0.0f) ? (size.z / size_.z) : 1.0f;

    // Scale all vertices by the scale factor
    for (auto& v : vertices) {
        v.x *= scaleFactor.x;
        v.y *= scaleFactor.y;
        v.z *= scaleFactor.z;
    }
    size_ = size;

    updateVboVertexData();
    computeBoundingBox();
}
*/

bool Mesh::intersectRay(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float& hitDist) const {
    // Compute AABB in world space
    glm::vec3 minV(FLT_MAX), maxV(-FLT_MAX);
    for (const auto& v : vertices) {
        glm::vec4 worldV = model_ * glm::vec4(v, 1.0f);
        minV = glm::min(minV, glm::vec3(worldV));
        maxV = glm::max(maxV, glm::vec3(worldV));
    }

    // Slab method for ray-AABB intersection
    float tmin = (minV.x - rayOrigin.x) / rayDir.x;
    float tmax = (maxV.x - rayOrigin.x) / rayDir.x;
    if (tmin > tmax) std::swap(tmin, tmax);

    float tymin = (minV.y - rayOrigin.y) / rayDir.y;
    float tymax = (maxV.y - rayOrigin.y) / rayDir.y;
    if (tymin > tymax) std::swap(tymin, tymax);

    if ((tmin > tymax) || (tymin > tmax))
        return false;

    if (tymin > tmin)
        tmin = tymin;
    if (tymax < tmax)
        tmax = tymax;

    float tzmin = (minV.z - rayOrigin.z) / rayDir.z;
    float tzmax = (maxV.z - rayOrigin.z) / rayDir.z;
    if (tzmin > tzmax) std::swap(tzmin, tzmax);

    if ((tmin > tzmax) || (tzmin > tmax))
        return false;

    if (tzmin > tmin)
        tmin = tzmin;
    if (tzmax < tmax)
        tmax = tzmax;

    if (tmax < 0) // Box is behind ray
        return false;

    hitDist = (tmin >= 0) ? tmin : tmax;
    return true;
}

