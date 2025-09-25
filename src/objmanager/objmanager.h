#pragma once
#include <string>
#include <unordered_map>
#include <map>
#include <tuple>
#include <vector>
#include <glm/glm.hpp>
#include "../texture/texture.h"

struct OBJData {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texcoords;
    std::vector<glm::vec3> normals;

    std::vector<unsigned int> indices;
    std::string texturePath;
    std::string mtlPath;
    std::string roughPath;
    std::string normalPath;

    // For normal mapping
    std::vector<glm::vec3> tangents;
    std::vector<glm::vec3> bitangents;
};

class OBJManager {
public:
    static OBJData getOBJ(const std::string& path, std::string texturePath="", std::string mtlPath="", std::string roughPath="", std::string normalPath="");
    static void clear();
    static std::vector<std::string> getLoadedOBJPaths();

private:
    static std::unordered_map<std::string, OBJData*> objs_;
    static void computeTangentsAndBitangents(OBJData& data);
};