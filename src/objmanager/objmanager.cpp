#include "objmanager.h"
#include <fstream>
#include <sstream>
#include <iostream>

std::unordered_map<std::string, OBJData*> OBJManager::objs_;

static void parseOBJ(const std::string& path, OBJData* data) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open OBJ file: " << path << std::endl;
        return;
    }

    std::string line;
    std::vector<glm::vec3> temp_vertices;
    std::vector<glm::vec2> temp_texcoords;
    std::vector<glm::vec3> temp_normals;
    std::vector<unsigned int> vertexIndices, texcoordIndices, normalIndices;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;
        if (type == "v") {
            glm::vec3 v;
            iss >> v.x >> v.y >> v.z;
            temp_vertices.push_back(v);
        } else if (type == "vt") {
            glm::vec2 tc;
            iss >> tc.x >> tc.y;
            tc.y = 1.0f - tc.y;
            temp_texcoords.push_back(tc);
        } else if (type == "vn") {
            glm::vec3 n;
            iss >> n.x >> n.y >> n.z;
            temp_normals.push_back(n);
        } else if (type == "f") {
            std::string vertexStr;
            std::vector<unsigned int> faceVertexIndices;
            std::vector<unsigned int> faceTexcoordIndices;
            std::vector<unsigned int> faceNormalIndices;
            while (iss >> vertexStr) {
                size_t firstSlash = vertexStr.find('/');
                size_t secondSlash = vertexStr.find('/', firstSlash + 1);
                unsigned int vIdx = std::stoi(vertexStr.substr(0, firstSlash));
                unsigned int tcIdx = 0, nIdx = 0;
                if (firstSlash != std::string::npos) {
                    size_t tcEnd = (secondSlash != std::string::npos) ? secondSlash : vertexStr.length();
                    std::string tcStr = vertexStr.substr(firstSlash + 1, tcEnd - firstSlash - 1);
                    if (!tcStr.empty())
                        tcIdx = std::stoi(tcStr);
                }
                if (secondSlash != std::string::npos) {
                    std::string nStr = vertexStr.substr(secondSlash + 1);
                    if (!nStr.empty())
                        nIdx = std::stoi(nStr);
                }
                faceVertexIndices.push_back(vIdx - 1);
                faceTexcoordIndices.push_back(tcIdx ? tcIdx - 1 : 0);
                faceNormalIndices.push_back(nIdx ? nIdx - 1 : 0);
            }
            for (size_t i = 1; i + 1 < faceVertexIndices.size(); ++i) {
                vertexIndices.push_back(faceVertexIndices[0]);
                vertexIndices.push_back(faceVertexIndices[i]);
                vertexIndices.push_back(faceVertexIndices[i + 1]);
                texcoordIndices.push_back(faceTexcoordIndices[0]);
                texcoordIndices.push_back(faceTexcoordIndices[i]);
                texcoordIndices.push_back(faceTexcoordIndices[i + 1]);
                normalIndices.push_back(faceNormalIndices[0]);
                normalIndices.push_back(faceNormalIndices[i]);
                normalIndices.push_back(faceNormalIndices[i + 1]);
            }
        }
    }

    std::map<std::tuple<unsigned int, unsigned int, unsigned int>, unsigned int> uniqueVertexMap;
    data->vertices.clear();
    data->texcoords.clear();
    data->normals.clear();
    data->indices.clear();

    for (size_t i = 0; i < vertexIndices.size(); ++i) {
        auto key = std::make_tuple(vertexIndices[i], texcoordIndices[i], normalIndices[i]);
        auto it = uniqueVertexMap.find(key);
        if (it != uniqueVertexMap.end()) {
            data->indices.push_back(it->second);
        } else {
            unsigned int newIndex = data->vertices.size();
            data->vertices.push_back(temp_vertices[vertexIndices[i]]);
            data->texcoords.push_back(temp_texcoords[texcoordIndices[i]]);
            data->normals.push_back(temp_normals[normalIndices[i]]);
            data->indices.push_back(newIndex);
            uniqueVertexMap[key] = newIndex;
        }
    }
}

OBJData OBJManager::getOBJ(const std::string& path, std::string texturePath, std::string mtlPath, std::string roughPath, std::string normalPath) {
    auto it = objs_.find(path);
    if (it != objs_.end()) {
        //std::cout << "cached obj found: " << path << std::endl;
        return *(it->second);
    }
    //std::cout << "loading obj: " << path << std::endl;
    OBJData* data = new OBJData();
    parseOBJ(path, data);
    data->texturePath = texturePath;
    data->mtlPath = mtlPath;
    data->roughPath = roughPath;
    data->normalPath = normalPath;
    if (!normalPath.empty()) {
        computeTangentsAndBitangents(*data);
    }
    objs_[path] = data;
    return *data;
}

void OBJManager::clear() {
    for (auto& pair : objs_) {
        delete pair.second;
    }
    objs_.clear();
}

std::vector<std::string> OBJManager::getLoadedOBJPaths() {
    std::vector<std::string> paths;
    for (const auto& pair : objs_) {
        paths.push_back(pair.first);
    }
    return paths;
}

void OBJManager::computeTangentsAndBitangents(OBJData& data) {
    size_t vertexCount = data.vertices.size();
    data.tangents.resize(vertexCount, glm::vec3(0.0f));
    data.bitangents.resize(vertexCount, glm::vec3(0.0f));

    for (size_t i = 0; i < data.indices.size(); i += 3) {
        unsigned int i0 = data.indices[i];
        unsigned int i1 = data.indices[i + 1];
        unsigned int i2 = data.indices[i + 2];

        const glm::vec3& v0 = data.vertices[i0];
        const glm::vec3& v1 = data.vertices[i1];
        const glm::vec3& v2 = data.vertices[i2];

        const glm::vec2& uv0 = data.texcoords[i0];
        const glm::vec2& uv1 = data.texcoords[i1];
        const glm::vec2& uv2 = data.texcoords[i2];

        glm::vec3 deltaPos1 = v1 - v0;
        glm::vec3 deltaPos2 = v2 - v0;
        glm::vec2 deltaUV1 = uv1 - uv0;
        glm::vec2 deltaUV2 = uv2 - uv0;

        float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
        glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
        glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;

        data.tangents[i0] += tangent;
        data.tangents[i1] += tangent;
        data.tangents[i2] += tangent;

        data.bitangents[i0] += bitangent;
        data.bitangents[i1] += bitangent;
        data.bitangents[i2] += bitangent;
    }

    // Normalize tangents and bitangents
    for (size_t i = 0; i < vertexCount; ++i) {
        data.tangents[i] = glm::normalize(data.tangents[i]);
        data.bitangents[i] = glm::normalize(data.bitangents[i]);
    }
}