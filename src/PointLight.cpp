// PointLight.cpp
#include "PointLight.h"

PointLight::PointLight(const glm::vec3& pos, float nearPlane, float farPlane)
    : position(pos), nearPlane(nearPlane), farPlane(farPlane) {

    glGenFramebuffers(1, &depthMapFBO);
    glGenTextures(1, &depthCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT,
            SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    update();
}

PointLight::~PointLight() {
    glDeleteFramebuffers(1, &depthMapFBO);
    glDeleteTextures(1, &depthCubemap);
}

void PointLight::update() {
    glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f),
        (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, nearPlane, farPlane);

    shadowTransforms.clear();
    shadowTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3(1, 0, 0), glm::vec3(0, -1, 0)));
    shadowTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3(-1, 0, 0), glm::vec3(0, -1, 0)));
    shadowTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)));
    shadowTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3(0, -1, 0), glm::vec3(0, 0, -1)));
    shadowTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3(0, 0, 1), glm::vec3(0, -1, 0)));
    shadowTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3(0, 0, -1), glm::vec3(0, -1, 0)));
}

void PointLight::setupShadowMatrices(Shader& shader) {
    for (size_t i = 0; i < 6; ++i) {
        shader.setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
    }
    shader.setFloat("far_plane", farPlane);
    shader.setVec3("lightPos", position);
}

const std::vector<glm::mat4>& PointLight::getShadowTransforms() const {
    return shadowTransforms;
}
