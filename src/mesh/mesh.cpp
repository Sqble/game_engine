#include "mesh.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <epoxy/gl.h>

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
    if (!this->isEffectivelyActive()) return;

    updateModelMatrix();

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

void Mesh::shadowDraw(Shader& shader) {
    if (!this->isEffectivelyActive()) return;

    updateModelMatrix();

    shader.setMat4("u_model", model_);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
}

bool Mesh::intersectRay(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float& hitDist) const {
    updateModelMatrix();

    // 1. Fast AABB check
    AABB worldAABB = getWorldAABB();
    // Ray-AABB intersection (slab method)
    glm::vec3 invDir = 1.0f / rayDir;
    glm::vec3 t0s = (worldAABB.min - rayOrigin) * invDir;
    glm::vec3 t1s = (worldAABB.max - rayOrigin) * invDir;
    glm::vec3 tsmaller = glm::min(t0s, t1s);
    glm::vec3 tbigger = glm::max(t0s, t1s);
    float tmin = std::max(std::max(tsmaller.x, tsmaller.y), tsmaller.z);
    float tmax = std::min(std::min(tbigger.x, tbigger.y), tbigger.z);
    if (tmax < 0 || tmin > tmax) return false;

    // 2. Mesh collider: ray-triangle intersection
    bool hit = false;
    float closestDist = FLT_MAX;
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        // Get triangle vertices in world space
        glm::vec3 v0 = glm::vec3(model_ * glm::vec4(vertices[indices[i]], 1.0f));
        glm::vec3 v1 = glm::vec3(model_ * glm::vec4(vertices[indices[i+1]], 1.0f));
        glm::vec3 v2 = glm::vec3(model_ * glm::vec4(vertices[indices[i+2]], 1.0f));

        // Möller–Trumbore intersection
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 h = glm::cross(rayDir, edge2);
        float a = glm::dot(edge1, h);
        if (fabs(a) < 1e-6) continue; // Parallel
        float f = 1.0f / a;
        glm::vec3 s = rayOrigin - v0;
        float u = f * glm::dot(s, h);
        if (u < 0.0f || u > 1.0f) continue;
        glm::vec3 q = glm::cross(s, edge1);
        float v = f * glm::dot(rayDir, q);
        if (v < 0.0f || u + v > 1.0f) continue;
        float t = f * glm::dot(edge2, q);
        if (t > 1e-6 && t < closestDist) {
            closestDist = t;
            hit = true;
        }
    }
    if (hit) {
        hitDist = closestDist;
        return true;
    }
    return false;
}
