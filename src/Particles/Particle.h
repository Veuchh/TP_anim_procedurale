#pragma once

#include <glm/gtc/matrix_transform.hpp>

class Particle
{
private :
	glm::vec3 currentPosition;
	glm::vec3 currentVelocity;

public:
	Particle();
	Particle(glm::vec3 startPosition, glm::vec3 startVelocity);
	~Particle();
	glm::vec3 SetNewPositionFromForce(float cubeSize, glm::vec3 externalForces, double deltaTime);
	glm::vec3 GetPosition();
	glm::vec3 GetVelocity();
};