#include "Well.h"
#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>

Well::Well(glm::vec3 startPositon, float startSize)
{
	wellPosition = startPositon;
	size = startSize;
}

Well::~Well()
{
}

glm::vec3 Well::GetPosition()
{
	return wellPosition;
}

float Well::GetSize()
{
	return size;
}

glm::vec3 Well::GetPullVectorFromPosition(glm::vec3 position)
{
	float distanceMultiplier = ((float)1 / (wellPosition - position).length()) * size;
	
	glm::vec3 normalizedVector = glm::normalize(wellPosition - position);

	return normalizedVector * distanceMultiplier;
}
