#pragma once

#include <glm/gtc/matrix_transform.hpp>

class Well
{
private:
	glm::vec3 wellPosition;
	float size;

public:
	Well(glm::vec3 startPositon, float startSize);
	~ Well();

	glm::vec3 GetPosition();
	float GetSize();

	glm::vec3 GetPullVectorFromPosition(glm::vec3 position);
};