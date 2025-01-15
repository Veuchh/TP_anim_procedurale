#include "Particle.h"

Particle::Particle()
{
	currentPosition = glm::vec3((float)0, (float)0, (float)0);
	currentVelocity = glm::vec3((float)1, (float)1, (float)1);
}

Particle::Particle(glm::vec3 startPosition, glm::vec3 startVelocity)
{
	currentPosition = startPosition;
	currentVelocity = startVelocity;
}

Particle::~Particle()
{
}

glm::vec3 Particle::SetNewPositionFromForce(float cubeSize, glm::vec3 externalForces, double deltaTime)
{
	//TODO : add external forces

	externalForces = glm::vec3(externalForces.x * deltaTime, externalForces.y * deltaTime, externalForces.z * deltaTime);
	currentVelocity += externalForces;

	glm::vec3 appliedVelocity = glm::vec3(currentVelocity.x * deltaTime, currentVelocity.y * deltaTime, currentVelocity.z * deltaTime);
    // Calculate the potential next position based on the current velocity
	glm::vec3 nextPosition = currentPosition + appliedVelocity;

	// Check if the potential next position would cross the boundary along the x-axis
	if (std::abs(nextPosition.x) > cubeSize / 2) {
		// Reverse the x-velocity component to make the particle bounce off the wall
		currentVelocity.x = -currentVelocity.x;
	}

	// Check if the potential next position would cross the boundary along the y-axis
	if (nextPosition.y > cubeSize || nextPosition.y < 0) {
		// Reverse the y-velocity component to make the particle bounce off the wall
		currentVelocity.y = -currentVelocity.y;
	}

	// Check if the potential next position would cross the boundary along the z-axis
	if (std::abs(nextPosition.z) > cubeSize / 2) {
		// Reverse the z-velocity component to make the particle bounce off the wall
		currentVelocity.z = -currentVelocity.z;
	}


	// Update the position based on the corrected velocity
	currentPosition += currentVelocity;


	currentPosition.x = glm::clamp(currentPosition.x, -cubeSize / 2, cubeSize / 2);
	currentPosition.z = glm::clamp(currentPosition.z, -cubeSize / 2, cubeSize / 2);
	currentPosition.y = glm::clamp(currentPosition.y, (float)0, cubeSize);


	return currentPosition;
}

glm::vec3 Particle::GetPosition()
{
	return currentPosition;
}

glm::vec3 Particle::GetVelocity()
{
	return currentVelocity;
}
