#pragma once
#include <glm/vec3.hpp>

struct ClothParticle {
    glm::vec3 position = { 0, 0, 0 };
    glm::vec3 positionOld = { 0, 0, 0 };
    glm::vec3 velocity = { 0, 0, 0 };
    glm::vec3 forces = { 0, 0, 0 };
    float mass = 1;
    bool moving = true;

    void update(float deltaTime) {
        if (!moving) return;
        positionOld = position;
        velocity += (forces / mass) * deltaTime;
        position += velocity * deltaTime;
    }

    void updateDerivatives(float deltaTime)
    {
        velocity = (position - positionOld) / deltaTime;
        forces = {};
    }

    void move(const glm::vec3 vectorToAdd) {
        if (!moving) return;
        position += vectorToAdd;
    }
};
