#include <common/model.hpp>
#include <iostream>

// Desativa otimizações agressivas de memória que causam crashes no WSL/Linux
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_SIMD
#include <common/stb_image.h>

Model::Model(const std::string& path) {
    loadModel(path);
}

void Model::Draw(GLuint shaderID) {
    for(unsigned int i = 0; i < meshes.size(); i++) {
        meshes[i].Draw(shaderID);
    }
}

void Model::loadModel(const std::string& path) {
    std::cout << "[LOG 1] A iniciar Assimp para abrir o ficheiro: " << path << std::endl;
    Assimp::Importer importer;
    
    const aiScene* scene = importer.ReadFile(path.c_str( ), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);

    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cout << "[ERRO] Falha no Assimp: " << importer.GetErrorString() << std::endl;
        return;
    }
    
    std::cout << "[LOG 2] Ficheiro aberto com sucesso! A preparar pasta." << std::endl;
    directory = path.substr(0, path.find_last_of('/'));
    
    processNode(scene->mRootNode, scene);
    std::cout << "[LOG 6] Modelo carregado na totalidade!" << std::endl;
}

void Model::processNode(aiNode *node, const aiScene *scene) {
    for(unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        std::cout << "[LOG 3] A extrair geometria da malha " << i << "..." << std::endl;
        meshes.push_back(processMesh(mesh, scene));
    }
    for(unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh *mesh, const aiScene *scene) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    // Vértices
    for(unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        if (mesh->HasNormals())
            vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        else
            vertex.Normal = glm::vec3(0.0f, 0.0f, 0.0f);

        if(mesh->mTextureCoords[0])
            vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        else
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        
        vertices.push_back(vertex);
    }

    // Índices
    for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for(unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    std::cout << "[LOG 4] Geometria extraida. A ler materiais da malha..." << std::endl;

    // Materiais
   glm::vec3 colorToSave = glm::vec3(0.7f, 0.7f, 0.7f); // Cor cinzenta por defeito

    if(mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        
        std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        aiColor4D diffuseColor;
        if(AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &diffuseColor)) {
            colorToSave = glm::vec3(diffuseColor.r, diffuseColor.g, diffuseColor.b);
        }
    }

    return Mesh(vertices, indices, textures,colorToSave);
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName) {
    std::vector<Texture> textures;
    unsigned int count = mat->GetTextureCount(type);
    
    std::cout << "[LOG 5] Foram encontradas " << count << " texturas neste material." << std::endl;
    
    for(unsigned int i = 0; i < count; i++) {
        aiString str;
        mat->GetTexture(type, i, &str);
        
        bool skip = false;
        for(unsigned int j = 0; j < textures_loaded.size(); j++) {
            if(std::strcmp(textures_loaded[j].path.c_str(), str.C_Str()) == 0) {
                textures.push_back(textures_loaded[j]);
                skip = true; 
                break;
            }
        }
        if(!skip) {
            Texture texture;
            texture.id = TextureFromFile(str.C_Str(), this->directory);
            texture.type = typeName;
            texture.path = str.C_Str();
            textures.push_back(texture);
            textures_loaded.push_back(texture); 
        }
    }
    return textures;
}

unsigned int TextureFromFile(const char *path, const std::string &directory) {
    std::string filename = std::string(path);
    filename = directory + '/' + filename;

    std::cout << "----> A carregar imagem: " << filename << std::endl;

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format = GL_RGB;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        std::cout << "----> Sucesso ao carregar a textura!" << std::endl;
    } else {
        std::cout << "----> [ERRO] Nao foi possivel encontrar ou ler a imagem!" << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}