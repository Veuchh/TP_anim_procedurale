#pragma once


#include "../viewer.h"
#include "../drawbuffer.h"
#include "../renderapi.h"

#include <string>
#include <iostream>
#include <vector>
#include <time.h>
#include <imgui.h>
#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>
#include "../MyViewer.cpp"
#include "../FK/Bone.h"

using namespace glm;

struct FabrikViewer : Viewer {

	vec2 mousePos;

	bool leftMouseButtonPressed;
	bool altKeyPressed;
	bool targetPosCircles = true;
	double cachedElapsedTime;
	float maxDistanceThreshold = .1f;
	int maxIterations = 50;
	vec3 targetPosition;
	vec3 targetPositionOffset;
	std::vector<vec3> targetPositions = std::vector<vec3>();
	Bone* rootBone;

	VertexShaderAdditionalData additionalShaderData;

	FabrikViewer() : Viewer("FabrikViewer", 1280, 720) {}

	float GetRandFloat()
	{
		return rand() / (float)RAND_MAX;
	}

	float GetRandSignedFloat()
	{
		return (GetRandFloat() - .5) * 2;
	}

	void init() override {
		mousePos = { 0.f, 0.f };
		leftMouseButtonPressed = false;
		altKeyPressed = false;

		additionalShaderData.Pos = { 0.,0.,0. };
		cachedElapsedTime = 0;
		targetPositionOffset = vec3(0, 0, 0);

		rootBone = new Bone(vec3(1, 0, 0), nullptr);
		Bone* currentParentBone = rootBone;
		targetPositions.push_back(vec3());

		for (size_t i = 0; i < 5; i++)
		{
			currentParentBone = currentParentBone->AddChildBone(
				glm::vec3(1, 0, 0),
				glm::quat(1, 0, 0, 0));

			targetPositions.push_back(vec3());
		}
	}

	void IterateBackwards()
	{
		targetPositions[targetPositions.size() - 1] = targetPosition;

		for (size_t i = targetPositions.size() - 1; i > 0; i--)
		{
			vec3 dir = normalize(targetPositions[i - 1] - targetPositions[i]) * (rootBone->GetBoneByChainNumber(i, 0)->GetRelativePos().length() * 1.f);
			//std::cout << dir.x << " | " << dir.y << " | " << dir.z << std::endl;
			targetPositions[i - 1] = targetPositions[i] + dir;
		}
	}

	void IterateForward()
	{
		targetPositions[0] = vec3(0);

		for (size_t i = 0; i < targetPositions.size() - 1; i++)
		{
			vec3 dir = normalize(targetPositions[i + 1] - targetPositions[i]) * (rootBone->GetBoneByChainNumber(i, 0)->GetRelativePos().length() * 1.f);
			//std::cout << dir.x << " | " << dir.y << " | " << dir.z << std::endl;
			targetPositions[i + 1] = targetPositions[i] + dir;
		}
	}

	void CalculateNewBonesPosition()
	{
		float chainLength = rootBone->GetChainLength();

		for (size_t i = 1; i < targetPositions.size(); i++)
		{
			Bone* boneByIndex = rootBone->GetBoneByChainNumber(i, 0);
			targetPositions[i] = boneByIndex->GetAbsolutePos();
		}

		if (chainLength < targetPosition.length())
		{
			//std::cout << "Too long for the chain" << std::endl;
			//Draw straight
			return;
		}

		float targetDistance = (targetPosition - targetPositions[targetPositions.size() - 1]).length();

		int currentIteration = 0;

		while (targetDistance > maxDistanceThreshold && currentIteration < maxIterations)
		{
			//std::cout << "Iteration number " << currentIteration <<std::endl;
			IterateBackwards();
			IterateForward();
			currentIteration++;
		}

		rootBone->SetRelativePos(targetPositions[0]);

		for (size_t i = 1; i < targetPositions.size(); i++)
		{
			//convert positions to relative
			//std::cout << targetPositions[i].x << " | " << targetPositions[i].y << " | " << targetPositions[i].z << std::endl;
			Bone* boneByIndex = rootBone->GetBoneByChainNumber(i, 0);

			if (boneByIndex != nullptr)
			{
				vec3 relativePos = targetPositions[i] - targetPositions[i - 1];
				boneByIndex->SetRelativePos(relativePos);
			}
			//apply positions
		}

	}

	void update(double elapsedTime) override {

		leftMouseButtonPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

		altKeyPressed = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;

		double mouseX;
		double mouseY;
		glfwGetCursorPos(window, &mouseX, &mouseY);

		mousePos = { float(mouseX), viewportHeight - float(mouseY) };

		pCustomShaderData = &additionalShaderData;
		CustomShaderDataSize = sizeof(VertexShaderAdditionalData);

		double deltaTime = elapsedTime - cachedElapsedTime;
		cachedElapsedTime = elapsedTime;

		targetPosition = targetPositionOffset;

		if (targetPosCircles)
			targetPosition += vec3(cos(elapsedTime), sin(elapsedTime), 0);

		CalculateNewBonesPosition();
		std::cout << std::endl;
		std::cout << std::endl;
		std::cout << std::endl;
		std::cout << std::endl;
		std::cout << std::endl;
	}

	void render3D_custom(const RenderApi3D& api) const override {
		//Here goes your drawcalls affected by the custom vertex shader
		//api.horizontalPlane({ 0, 2, 0 }, { 4, 4 }, 200, vec4(0.0f, 0.2f, 1.f, 1.f));
	}

	void RenderBoneRecursive(const RenderApi3D& api, Bone* boneToRender, vec3 parentBoneAbsPos, quat parentBoneAbsRot) const
	{
		vec3 currentBoneAbsPos = boneToRender->GetAbsolutePos(parentBoneAbsPos, parentBoneAbsRot);
		quat currentBoneAbsRot = boneToRender->GetAbsoluteRot(parentBoneAbsRot);

		api.bone(boneToRender->GetRelativePos(), white, parentBoneAbsRot, parentBoneAbsPos);

		for each (Bone * childBone in boneToRender->GetChildBones())
		{
			RenderBoneRecursive(api, childBone, currentBoneAbsPos, currentBoneAbsRot);
		}
	}

	void render3D(const RenderApi3D& api) const override
	{
		api.solidSphere(targetPosition, .2f, 15, 15, white);
		RenderBoneRecursive(api, rootBone, vec3(0, 0, 0), quat(1, 0, 0, 0));
	}

	void render2D(const RenderApi2D& api) const override {

		constexpr float padding = 50.f;

		if (altKeyPressed) {
			if (leftMouseButtonPressed) {
				api.circleFill(mousePos, padding, 10, white);
			}
			else {
				api.circleContour(mousePos, padding, 10, white);
			}
		}

		else {
			const vec2 min = mousePos + vec2(padding, padding);
			const vec2 max = mousePos + vec2(-padding, -padding);
			if (leftMouseButtonPressed) {
				api.quadFill(min, max, white);
			}
			else {
				api.quadContour(min, max, white);
			}
		}
	}

	void drawGUI() override {
		static bool showDemoWindow = false;

		ImGui::Begin("FK");

		ImGui::Checkbox("Show demo window", &showDemoWindow);

		ImGui::ColorEdit4("Background color", (float*)&backgroundColor, ImGuiColorEditFlags_NoInputs);

		ImGui::DragInt("Max iterations", &maxIterations, -10.f, 10.f);
		ImGui::DragFloat("Max Distance Threshold", &maxDistanceThreshold, -10.f, 10.f);
		ImGui::Checkbox("TargetPosCircles", &targetPosCircles);
		ImGui::SliderFloat3("Target position offset", reinterpret_cast<float(&)[3]>(targetPositionOffset), -10.f, 10.f);
		if (ImGui::Button("Reset offset"))
		{
			targetPositionOffset = vec3(0);
		}

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		ImGui::End();

		if (showDemoWindow) {
			// Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
			ImGui::ShowDemoWindow(&showDemoWindow);
		}
	}
};
