#pragma once
#include <glm/glm.hpp>
#include <algorithm>

class AABB {
public:
    glm::vec3 min;
    glm::vec3 max;

    AABB() : min(0.0f), max(0.0f) {}
    AABB(const glm::vec3& min_, const glm::vec3& max_) : min(min_), max(max_) {}

    // Expand the box to include a point
    void expand(const glm::vec3& point) {
        min = glm::min(min, point);
        max = glm::max(max, point);
    }

    // Transform the AABB by a model matrix (returns a new AABB)
    AABB transformed(const glm::mat4& mat) const {
        glm::vec3 corners[8] = {
            {min.x, min.y, min.z}, {max.x, min.y, min.z},
            {min.x, max.y, min.z}, {max.x, max.y, min.z},
            {min.x, min.y, max.z}, {max.x, min.y, max.z},
            {min.x, max.y, max.z}, {max.x, max.y, max.z}
        };
        glm::vec3 newMin = glm::vec3(mat * glm::vec4(corners[0], 1.0f));
        glm::vec3 newMax = newMin;
        for (int i = 1; i < 8; ++i) {
            glm::vec3 v = glm::vec3(mat * glm::vec4(corners[i], 1.0f));
            newMin = glm::min(newMin, v);
            newMax = glm::max(newMax, v);
        }
        return AABB(newMin, newMax);
    }

    // Check if this AABB is outside a single plane (plane: vec4(xyz=normal, w=distance))
    bool isOutsidePlane(const glm::vec4& plane) const {
        // Compute the positive vertex (farthest in direction of plane normal)
        glm::vec3 positive = min;
        if (plane.x >= 0) positive.x = max.x;
        if (plane.y >= 0) positive.y = max.y;
        if (plane.z >= 0) positive.z = max.z;
        // If positive vertex is outside, the box is outside
        return glm::dot(glm::vec3(plane), positive) + plane.w < 0;
    }
};