#pragma once

#include "ClothParticle.hpp"

struct ClothConstraint {
    std::reference_wrapper<ClothParticle> particle1;
    std::reference_wrapper<ClothParticle> particle2;
    float distance = 1.0f;
    float strength = 1.0f;
    float maxElongationRatio = 1.5f;
    bool broken = false;

    ClothConstraint(ClothParticle& p1, ClothParticle& p2, const float distance, const float strength, const float maxElongationRatio)
        : particle1(p1), particle2(p2), distance(distance), strength(strength), maxElongationRatio(maxElongationRatio) {}

    ClothConstraint(ClothParticle& p1, ClothParticle& p2, float strength, float maxElongationRation) : particle1(p1),
    particle2(p2), strength(strength), maxElongationRatio(maxElongationRation) {
        distance = glm::length(particle1.get().position - particle2.get().position);
    }

    bool isValid() const{
        return !broken;
    }

    void solve()
    {
        if (!isValid()) { return; }
        const glm::vec3 v = particle1.get().position - particle2.get().position;
        const float dist = length(v);
        if (dist > distance) {
            // Break the constraint if the distance is too high
            broken = dist > distance * maxElongationRatio;
            const glm::vec3 n = v / dist;
            const float c = distance - dist;
            const glm::vec3 p = -(c * strength) / (particle1.get().mass + particle2.get().mass) * n;

            // Apply position correction
            particle1.get().move(-p / particle1.get().mass);
            particle2.get().move( p / particle2.get().mass);
        }
    }
};
