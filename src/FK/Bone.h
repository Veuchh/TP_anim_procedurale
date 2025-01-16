#pragma once

#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>

class Bone {
private:
	glm::vec3 relativePos;
	glm::quat relativeRot;
	std::vector<Bone*> childBones = std::vector<Bone*>();

public:
	glm::vec3 relativeRotEuler = glm::vec3(0,0,0);
	Bone(glm::quat startRelativeRot, glm::vec3 startRelativePos);
	~Bone();

	glm::vec3 GetRelativePos();
	glm::quat GetRelativeRot();
	std::vector<Bone*> GetChildBones();

	glm::vec3 GetAbsolutePos(glm::vec3 parentAbsolutePos, glm::quat parentAbsoluteRot);
	glm::quat GetAbsoluteRot(glm::quat parentAbsoluteRot);
	glm::quat SetRelativeRot(glm::vec3 eulerRot);


	Bone* AddChildBone(glm::vec3 newBoneRelPos, glm::quat newBoneRelRot);
};