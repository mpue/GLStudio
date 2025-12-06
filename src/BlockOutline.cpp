#include "BlockOutline.h"
#include <vector>
#include <glm/gtc/matrix_transform.hpp>

BlockOutline::BlockOutline() : VAO(0), VBO(0), EBO(0), shader(nullptr), initialized(false) {
    setupOutlineMesh();
}

BlockOutline::~BlockOutline() {
    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    }
    if (shader != nullptr) {
        delete shader;
    }
}

void BlockOutline::init(const char* vertPath, const char* fragPath) {
    if (!initialized) {
        shader = new Shader(vertPath, fragPath);
      initialized = true;
    }
}

void BlockOutline::setupOutlineMesh() {
 // Würfel-Vertices (8 Ecken eines 1x1x1 Würfels)
    float vertices[] = {
        // Untere 4 Ecken
-0.5f, -0.5f, -0.5f,  // 0
         0.5f, -0.5f, -0.5f,  // 1
         0.5f, -0.5f,  0.5f,  // 2
        -0.5f, -0.5f,  0.5f,  // 3
        
        // Obere 4 Ecken
   -0.5f,  0.5f, -0.5f,// 4
  0.5f,  0.5f, -0.5f,  // 5
         0.5f,  0.5f,  0.5f,  // 6
        -0.5f,  0.5f,  0.5f   // 7
    };

    // Indices für 12 Linien (Kanten des Würfels)
    unsigned int indices[] = {
        // Untere Fläche
        0, 1,  1, 2,  2, 3,  3, 0,
 // Obere Fläche
        4, 5,  5, 6,  6, 7,  7, 4,
        // Vertikale Kanten
        0, 4,  1, 5,  2, 6,  3, 7
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

  glBindVertexArray(0);
}

void BlockOutline::renderOutline(const glm::ivec3& blockPos, const glm::mat4& projection, 
    const glm::mat4& view, const glm::vec3& color, float alpha) {

    if (!initialized || shader == nullptr) {
        return;
    }

    // Erstelle Model-Matrix
    // Block-Koordinaten sind bereits zentriert (Block bei (x,y,z) reicht von x-0.5 bis x+0.5)
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(blockPos));
  model = glm::scale(model, glm::vec3(1.005f)); // Leicht größer für sichtbare Outline

    // Konfiguriere Render-State
glLineWidth(3.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST); // Outline immer sichtbar
    
    // Setze Shader-Uniforms
    shader->use();
 shader->setMat4("projection", projection);
    shader->setMat4("view", view);
    shader->setMat4("model", model);
    shader->setVec3("outlineColor", color);
    shader->setFloat("alpha", alpha);
    
    // Render
    glBindVertexArray(VAO);
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    
    // Restore state
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glLineWidth(1.0f);
}

void BlockOutline::renderDualOutline(const glm::ivec3& removePos, const glm::ivec3& placePos,
    const glm::mat4& projection, const glm::mat4& view) {
 
    // Render remove outline (Rot, weniger transparent)
    renderOutline(removePos, projection, view, glm::vec3(1.0f, 0.2f, 0.2f), 0.9f);
 
    // Render place outline (Grün, transparenter)
    renderOutline(placePos, projection, view, glm::vec3(0.2f, 1.0f, 0.2f), 0.7f);
}
