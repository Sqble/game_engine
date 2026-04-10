#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "../shader/shader.h"
#include "../texture/texture.h"
#include "../sceneobject/drawableobject.h"
#include <iostream>
#include "../aabb/aabb.h"
#include "../material/material.h"

class Mesh : public DrawableObject {
public:
    Mesh(const std::string& objPath,
         const Material& mat = Material(),
         const glm::vec3& position = glm::vec3(0.0f),
         const glm::vec3& size = glm::vec3(1.0f),
         const glm::vec3& color = glm::vec3(1.0f),
         const glm::vec3& rotation = glm::vec3(0.0f),
         bool isActive = true);
    void draw(Shader& shader) override;
    void shadowDraw(Shader& shader);
    bool intersectRay(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float& hitDist) const override;
    bool isMesh() const override { return true; }

    AABB getWorldAABB() const {
        updateModelMatrix();
        return localAABB_.transformed(model_);
    }

    void computeBoundingBox() {
    // Compute bounding box
        if (!vertices.empty()) {
            glm::vec3 minV = vertices[0];
            glm::vec3 maxV = vertices[0];
            for (const auto& v : vertices) {
                minV = glm::min(minV, v);
                maxV = glm::max(maxV, v);
            }
            localAABB_ = AABB(minV, maxV);
        }
    }

    void updateVboVertexData();

    std::string objPath;
    Material material;
private:
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texcoords;
    std::vector<unsigned int> indices;
    std::vector<float> bufferData;
    std::vector<glm::vec3> normals;

    //for normal map
    std::vector<glm::vec3> tangents;
    std::vector<glm::vec3> bitangents;

    AABB localAABB_;
};
