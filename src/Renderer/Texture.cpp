#include "Texture.h"
#include "../Core/Settings.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../external/stb/stb_image.h"
#include <iostream>
#include <algorithm>

Texture::Texture() : ID(0), width(0), height(0), nrChannels(0) {
    glGenTextures(1, &ID);
}

bool Texture::loadHDR(const char* path) {
    stbi_set_flip_vertically_on_load(true);
    float* data = stbi_loadf(path, &width, &height, &nrChannels, 0);
    if (data) {
        glBindTexture(GL_TEXTURE_2D, ID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data); 

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        return true;
    } else {
        std::cout << "Failed to load HDR texture: " << path << std::endl;
        return false;
    }
}

Texture::~Texture() {
    glDeleteTextures(1, &ID);
}

bool Texture::loadFromFile(const char* path, bool gammaCorrection) {
    stbi_set_flip_vertically_on_load(true);
    
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data) {
        GLenum internalFormat = GL_RGB;
        GLenum dataFormat = GL_RGB;
        bool supportedFormat = true;

        if (nrChannels == 1) {
            internalFormat = GL_RED;
            dataFormat = GL_RED;
        } else if (nrChannels == 3) {
            internalFormat = gammaCorrection ? GL_SRGB : GL_RGB;
            dataFormat = GL_RGB;
        } else if (nrChannels == 4) {
            internalFormat = gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
            dataFormat = GL_RGBA;
        } else {
            supportedFormat = false;
        }

        if (!supportedFormat) {
            std::cerr << "Unsupported texture channel count: " << nrChannels
                      << " for file " << path << std::endl;
            stbi_image_free(data);
            return false;
        }

        glBindTexture(GL_TEXTURE_2D, ID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        // Anisotropic Filtering
        GLfloat maxAniso = 0.0f;
        glGetFloatv(0x84FF /*GL_MAX_TEXTURE_MAX_ANISOTROPY*/, &maxAniso); 
        
        float desiredAniso = (float)Settings::getInstance().graphics.anisotropicLevel;
        float finalAniso = std::min(maxAniso, desiredAniso);
        glTexParameterf(GL_TEXTURE_2D, 0x84FE /*GL_TEXTURE_MAX_ANISOTROPY*/, finalAniso);
        
        stbi_image_free(data);
        return true;
    }
    else {
        std::cout << "Failed to load texture: " << path << std::endl;
        stbi_image_free(data);
        return false;
    }
}

void Texture::bind(unsigned int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, ID);
}

bool Texture::loadCubemap(const std::vector<std::string>& faces) {
    glBindTexture(GL_TEXTURE_CUBE_MAP, ID);

    stbi_set_flip_vertically_on_load(false); // Cubemaps typically don't need flipping
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            GLenum format = GL_RGB;
            if (nrChannels == 1) format = GL_RED;
            else if (nrChannels == 3) format = GL_RGB;
            else if (nrChannels == 4) format = GL_RGBA;

            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                         0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        } else {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
            return false;
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return true;
}

void Texture::bindCubemap(unsigned int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, ID);
}
