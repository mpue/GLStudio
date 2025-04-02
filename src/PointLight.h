// PointLight.h
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <shader.h>

class PointLight {
public:
    glm::vec3 position;
    float nearPlane;
    float farPlane;
    unsigned int depthMapFBO;
    unsigned int depthCubemap;
    static constexpr unsigned int SHADOW_WIDTH = 1024;
    static constexpr unsigned int SHADOW_HEIGHT = 1024;

    PointLight(const glm::vec3& pos, float nearPlane = 1.0f, float farPlane = 25.0f);
    ~PointLight();
    void update();
    void setupShadowMatrices(Shader& shader);
    const std::vector<glm::mat4>& getShadowTransforms() const;
private:
    std::vector<glm::mat4> shadowTransforms;
};
