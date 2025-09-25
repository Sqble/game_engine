#pragma once
#include <glm/glm.hpp>
#include "../aabb/aabb.h"
#include <array>

class Frustum {
public:
    // 0: left, 1: right, 2: bottom, 3: top, 4: near, 5: far
    std::array<glm::vec4, 6> planes;

    // Extract planes from combined view-projection matrix
    void extract(const glm::mat4& vp) {
        // Left
        planes[0] = glm::vec4(
            vp[0][3] + vp[0][0],
            vp[1][3] + vp[1][0],
            vp[2][3] + vp[2][0],
            vp[3][3] + vp[3][0]);
        // Right
        planes[1] = glm::vec4(
            vp[0][3] - vp[0][0],
            vp[1][3] - vp[1][0],
            vp[2][3] - vp[2][0],
            vp[3][3] - vp[3][0]);
        // Bottom
        planes[2] = glm::vec4(
            vp[0][3] + vp[0][1],
            vp[1][3] + vp[1][1],
            vp[2][3] + vp[2][1],
            vp[3][3] + vp[3][1]);
        // Top
        planes[3] = glm::vec4(
            vp[0][3] - vp[0][1],
            vp[1][3] - vp[1][1],
            vp[2][3] - vp[2][1],
            vp[3][3] - vp[3][1]);
        // Near
        planes[4] = glm::vec4(
            vp[0][3] + vp[0][2],
            vp[1][3] + vp[1][2],
            vp[2][3] + vp[2][2],
            vp[3][3] + vp[3][2]);
        // Far
        planes[5] = glm::vec4(
            vp[0][3] - vp[0][2],
            vp[1][3] - vp[1][2],
            vp[2][3] - vp[2][2],
            vp[3][3] - vp[3][2]);
        // Normalize planes
        for (auto& p : planes) {
            float len = glm::length(glm::vec3(p));
            if (len > 0.0f) p /= len;
        }
    }

    // Returns true if the box is inside or intersects the frustum
    bool isBoxVisible(const AABB& box) const {
        for (const auto& plane : planes) {
            if (box.isOutsidePlane(plane))
                return false;
        }
        return true;
    }
};