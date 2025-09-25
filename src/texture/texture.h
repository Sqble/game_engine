#pragma once
#include <string>
#include <GL/glew.h>
#include <unordered_map>
#include <iostream>

class Texture {
public:
    Texture(const std::string& path);
    ~Texture();
    void bind(unsigned int unit = 0) const;
    GLuint getID() const { return textureID_; }
    std::string getPath() const { return path_; }

private:
    GLuint textureID_;
    std::string path_;
};

// texture manager
class TextureManager {
public:
    static Texture* getTexture(const std::string& path) {
        auto it = textures_.find(path);
        if (it != textures_.end()) {
            //std::cout << "cached texture found: " << path << std::endl;
            return it->second;
        } else {
            //std::cout << "loading texture: " << path << std::endl;
            Texture* tex = new Texture(path);
            textures_[path] = tex;
            return tex;
        }
    }

    static void clear() {
        for (auto& pair : textures_) {
            delete pair.second;
        }
        textures_.clear();
    }

private:
    static std::unordered_map<std::string, Texture*> textures_;
};