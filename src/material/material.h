#pragma once

#include <glm/glm.hpp>
#include <string>

#include "../texture/texture.h"



class Material {
    public:
        Material(glm::vec3 albedoColor = glm::vec3(1.0f),
                float roughness = 1.0f,
                float metalness = 0.0f)
            : albedoColor_(albedoColor), roughness_(roughness), metalness_(metalness) {}
        
        Material(std::string albedoPath,
                std::string roughPath = "",
                std::string metalPath = "",
                std::string normalPath = "",
                glm::vec3 albedoColor = glm::vec3(1.0f),
                float roughness = 1.0f,
                float metalness = 0.0f)
            : albedoColor_(albedoColor), roughness_(roughness), metalness_(metalness) {
                albedoPath_ = albedoPath;
                normalPath_ = normalPath;
                roughPath_ = roughPath;
                metalPath_ = metalPath;

                if (!albedoPath.empty()) {
                    albedoMap_ = TextureManager::getTexture(albedoPath);
                    hasAlbedoMap_ = (albedoMap_ != nullptr);
                }
                if (!normalPath.empty()) {
                    normalMap_ = TextureManager::getTexture(normalPath);
                    hasNormalMap_ = (normalMap_ != nullptr);
                }
                if (!roughPath.empty()) {
                    roughnessMap_ = TextureManager::getTexture(roughPath);
                    hasRoughnessMap_ = (roughnessMap_ != nullptr);
                }
                if (!metalPath.empty()) {
                    metalnessMap_ = TextureManager::getTexture(metalPath);
                    hasMetalnessMap_ = (metalnessMap_ != nullptr);
                }
            }
        
        // Getters/Setters
        glm::vec3 getAlbedoColor() const { return albedoColor_; }
        void setAlbedoColor(const glm::vec3& color) { albedoColor_ = color; }
        float getRoughness() const { return roughness_; }
        void setRoughness(float r) { roughness_ = r; }
        float getMetalness() const { return metalness_; }
        void setMetalness(float m) { metalness_ = m; }
        bool hasAlbedoMap() const { return hasAlbedoMap_; }
        bool hasNormalMap() const { return hasNormalMap_; }
        bool hasRoughnessMap() const { return hasRoughnessMap_; }
        bool hasMetalnessMap() const { return hasMetalnessMap_; }
        Texture* getAlbedoMap() const { return albedoMap_; }
        Texture* getNormalMap() const { return normalMap_; }
        Texture* getRoughnessMap() const { return roughnessMap_; }
        Texture* getMetalnessMap() const { return metalnessMap_; }
        void setAlbedoMap(const std::string& path) {
            if (path.empty()) {
                albedoMap_ = nullptr;
                hasAlbedoMap_ = false;
                return;
            }
            albedoMap_ = TextureManager::getTexture(path);
            hasAlbedoMap_ = (albedoMap_ != nullptr);
            albedoPath_ = path;
        }
        void setNormalMap(const std::string& path) {
            if (path.empty()) {
                normalMap_ = nullptr;
                hasNormalMap_ = false;
                return;
            }
            normalMap_ = TextureManager::getTexture(path);
            hasNormalMap_ = (normalMap_ != nullptr);
            normalPath_ = path;
        }
        void setRoughnessMap(const std::string& path) {
            if (path.empty()) {
                roughnessMap_ = nullptr;
                hasRoughnessMap_ = false;
                return;
            }
            roughnessMap_ = TextureManager::getTexture(path);
            hasRoughnessMap_ = (roughnessMap_ != nullptr);
            roughPath_ = path;
        }
        void setMetalnessMap(const std::string& path) {
            if (path.empty()) {
                metalnessMap_ = nullptr;
                hasMetalnessMap_ = false;
                return;
            }
            metalnessMap_ = TextureManager::getTexture(path);
            hasMetalnessMap_ = (metalnessMap_ != nullptr);
            metalPath_ = path;
        }

        std::string getAlbedoPath() const { return albedoPath_; }
        std::string getNormalPath() const { return normalPath_; }
        std::string getRoughPath() const { return roughPath_; }
        std::string getMetalPath() const { return metalPath_; }

    private:
        Texture *albedoMap_ = nullptr;
        Texture *normalMap_ = nullptr;
        Texture *roughnessMap_ = nullptr;
        Texture *metalnessMap_ = nullptr;

        std::string albedoPath_ = "";
        std::string normalPath_ = "";
        std::string roughPath_ = "";
        std::string metalPath_ = "";

        bool hasAlbedoMap_ = false;
        bool hasNormalMap_ = false;
        bool hasRoughnessMap_ = false;
        bool hasMetalnessMap_ = false;

        glm::vec3 albedoColor_ = glm::vec3(1.0f);
        float roughness_ = 1.0f;
        float metalness_ = 0.0f;
};