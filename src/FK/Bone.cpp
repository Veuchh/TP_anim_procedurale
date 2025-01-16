#include "Bone.h"

Bone::Bone(glm::quat startRelativeRot, glm::vec3 startRelativePos, Bone* newParentBone)
{
	relativeRot = startRelativeRot;
	relativePos = startRelativePos;
	parentBone = newParentBone;
}

Bone::Bone(glm::vec3 startRelativePos, Bone* newParentBone)
{
	relativePos = startRelativePos;
	relativeRot = glm::quat(1, 0, 0, 0);
	parentBone = newParentBone;
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

glm::vec3 Bone::GetAbsolutePos()
{
	if (parentBone == nullptr)
	{
		return glm::vec3(0);
	}
	return parentBone->GetAbsolutePos() + parentBone->GetAbsoluteRot() * relativePos;
}

glm::vec3 Bone::GetAbsolutePos(glm::vec3 parentAbsolutePos, glm::quat parentAbsoluteRot)
{
	return parentAbsolutePos + parentAbsoluteRot * relativePos;
}

glm::quat Bone::GetAbsoluteRot()
{
	if (parentBone == nullptr)
	{
		return glm::quat(1,0,0,0);
	}
	return parentBone->GetAbsoluteRot() * relativeRot;
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

void Bone::SetRelativePos(glm::vec3 newPos)
{
	relativePos = newPos;
}

Bone* Bone::GetBoneByChainNumber(int targetBoneNumber, int currentBoneID)
{
	if (currentBoneID == targetBoneNumber)
	return this;
	
	currentBoneID++;

	if (childBones.size() != 0)
		return childBones[0]->GetBoneByChainNumber(targetBoneNumber, currentBoneID);
	return nullptr;
}

float Bone::GetChainLength()
{
	return (childBones.size() > 0 ? childBones[0]->GetChainLength() : 0 ) + relativePos.length();
}

Bone* Bone::AddChildBone(glm::vec3 newBoneRelPos, glm::quat newBoneRelRot)
{
	Bone* newBone = new Bone(newBoneRelRot, newBoneRelPos, this);

	childBones.push_back(newBone);

	return newBone;
}

glm::vec3 Bone::GetRelativePos()
{
	return relativePos;
}
