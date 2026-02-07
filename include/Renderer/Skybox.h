#pragma once

#include <vector>
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include "Texture.h"
#include "Shader.h"

class Skybox {
public:
    Skybox(const std::vector<std::string>& faces);
    Skybox(const std::string& hdrPath, Shader& conversionShader);
    ~Skybox();

    void render(const glm::mat4& projection, const glm::mat4& view, Shader& shader);

private:
    unsigned int skyboxVAO, skyboxVBO;
    std::unique_ptr<Texture> cubemapTexture;
    void setupMesh();
    void bakeHDR(const std::string& hdrPath, Shader& conversionShader);
};
