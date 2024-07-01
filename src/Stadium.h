#pragma once

#include "GameObject.h"

class Stadium : public GameObject {
public:
    Stadium(unsigned int vao, unsigned int vbo, unsigned int ebo, const glm::vec3& pos, const glm::vec3& col,
            const glm::vec3& ringColor, const glm::vec3& crossColor, float radius, float curvature, int numRings,
            int verticesPerRing, float textureScale = 1.0f);

    void update() override {}
    void initializeMesh() override;
    void render(ShaderProgram &shader, const glm::vec3 &viewPos, const glm::vec3 &lightColor, const glm::vec3 &lightPos) override;

protected:
    void generateMeshData() override;

private:
    float radius;
    float curvature;
    int verticesPerRing;
    int numRings;
    glm::vec3 ringColor;
    glm::vec3 crossColor;

    float textureScale = 1.0f;
};