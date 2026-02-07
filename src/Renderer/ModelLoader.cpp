#include "ModelLoader.h"
#include <iostream>

std::vector<std::unique_ptr<Mesh>> ModelLoader::loadModel(const std::string& path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, 
        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    std::vector<std::unique_ptr<Mesh>> meshes;

    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return meshes;
    }

    processNode(scene->mRootNode, scene, meshes);
    return meshes;
}

void ModelLoader::processNode(aiNode* node, const aiScene* scene, std::vector<std::unique_ptr<Mesh>>& meshes) {
    // Process all the node's meshes (if any)
    for(unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]]; 
        meshes.push_back(processMesh(mesh, scene));
    }
    // Then do the same for each of its children
    for(unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, meshes);
    }
}

std::unique_ptr<Mesh> ModelLoader::processMesh(aiMesh* mesh, const aiScene* scene) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // Walk through each of the mesh's vertices
    for(unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        glm::vec3 vector; 

        // positions
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.Position = vector;

        // normals
        if (mesh->HasNormals()) {
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.Normal = vector;
        } else {
            vertex.Normal = glm::vec3(0.0f);
        }

        // texture coordinates
        if(mesh->mTextureCoords[0]) {
            glm::vec2 vec;
            vec.x = mesh->mTextureCoords[0][i].x; 
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;
        } else {
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        }

        vertices.push_back(vertex);
    }

    // Walk through each of the mesh's faces
    for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for(unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    return std::make_unique<Mesh>(vertices, indices);
}
