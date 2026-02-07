#pragma once

#include <string>
#include <vector>
#include <glad/gl.h>

class Texture {
public:
    unsigned int ID;
    std::string type;
    
    Texture();
    ~Texture();
    
    bool loadFromFile(const char* path, bool gammaCorrection = false);
    bool loadHDR(const char* path);
    bool loadCubemap(const std::vector<std::string>& faces);
    void bind(unsigned int unit = 0) const;
    void bindCubemap(unsigned int unit = 0) const;

    int getWidth() const { return width; }
    int getHeight() const { return height; }
    
private:
    int width, height, nrChannels;
};
