#include "Bone.h"

Bone::Bone(glm::quat startRelativeRot, glm::vec3 startRelativePos)
{
	relativeRot = startRelativeRot;
	relativePos = startRelativePos;
}

Bone::~Bone()
{
}

glm::quat EulerToQuat(glm::vec3 eulerRot) {
	glm::quat x = glm::angleAxis(eulerRot.x, glm::vec3(1.f, 0.f, 0.f));
	glm::quat y = glm::angleAxis(eulerRot.y, glm::vec3(0.f, 1.f, 0.f));
	glm::quat z = glm::angleAxis(eulerRot.z, glm::vec3(0.f, 0.f, 1.f));

	return x * y * z;
}

glm::quat Bone::GetRelativeRot()
{
	return relativeRot;
}

std::vector<Bone*> Bone::GetChildBones()
{
	return childBones;
}

glm::vec3 Bone::GetAbsolutePos(glm::vec3 parentAbsolutePos, glm::quat parentAbsoluteRot)
{
	return parentAbsolutePos + parentAbsoluteRot * relativePos;
}

glm::quat Bone::GetAbsoluteRot(glm::quat parentAbsoluteRot)
{
	return parentAbsoluteRot * relativeRot;
}

glm::quat Bone::SetRelativeRot(glm::vec3 eulerRot)
{
	relativeRot = EulerToQuat(eulerRot);
	relativeRotEuler = eulerRot;
	return GetRelativeRot();
}

Bone* Bone::AddChildBone(glm::vec3 newBoneRelPos, glm::quat newBoneRelRot)
{
	Bone* newBone = new Bone(newBoneRelRot, newBoneRelPos);

	childBones.push_back(newBone);

	return newBone;
}

glm::vec3 Bone::GetRelativePos()
{
	return relativePos;
}
