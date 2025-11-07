#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <shader.h>

class BlockOutline {
public:
    BlockOutline();
    ~BlockOutline();

    // Initialisiert den Outline-Shader
    void init(const char* vertPath, const char* fragPath);

    // Rendert eine Outline um einen Block
    void renderOutline(const glm::ivec3& blockPos, const glm::mat4& projection, 
        const glm::mat4& view, const glm::vec3& color, float alpha = 0.8f);
    
    // Rendert zwei Outlines (für Remove = Rot, Place = Grün)
    void renderDualOutline(const glm::ivec3& removePos, const glm::ivec3& placePos,
        const glm::mat4& projection, const glm::mat4& view);

private:
    unsigned int VAO, VBO, EBO;
    Shader* shader;
    bool initialized;
    
    void setupOutlineMesh();
};
