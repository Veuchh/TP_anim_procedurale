#pragma once


#include "../viewer.h"
#include "../drawbuffer.h"
#include "../renderapi.h"

#define _USE_MATH_DEFINES

#include <math.h>
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

struct SpiderViewer : Viewer {

	vec2 mousePos;

	bool leftMouseButtonPressed;
	bool altKeyPressed;
	bool targetPosSpheres = false;
	double cachedElapsedTime;
	float maxDistanceThreshold = .1f;
	float spiderHeight = 1.f;
	float walkSpeed = 5.f;
	float walkHeight = 2.f;
	int maxIterations = 1;
	vec3 targetPosition1;
	vec3 targetPosition2;
	vec3 targetPositionOffset;
	std::vector<vec3> targetPositions = std::vector<vec3>();
	Bone* rootBone1;
	Bone* rootBone2;
	Bone* rootBone3;
	Bone* rootBone4;
	Bone* rootBone5;
	Bone* rootBone6;
	Bone* rootBone7;
	Bone* rootBone8;

	float boneLength = 2.f;

	vec3 bone1Offset = vec3(-1.0f, 0.f, 3.5f);
	vec3 bone2Offset = vec3(-2.0f, 0.f, 2.5f);
	vec3 bone3Offset = vec3(-2.5f, 0.f, -0.f);
	vec3 bone4Offset = vec3(-1.0f, 0.f, -1.f);

	vec3 bone5Offset = vec3(1.0f, 0.f, 3.5f);
	vec3 bone6Offset = vec3(2.0f, 0.f, 2.5f);
	vec3 bone7Offset = vec3(2.5f, 0.f, -0.f);
	vec3 bone8Offset = vec3(1.0f, 0.f, -1.f);

	vec4 spiderColor = vec4(.16f, .06f, .055f, 1.f);
	VertexShaderAdditionalData additionalShaderData;

	SpiderViewer() : Viewer("SpiderViewer", 1280, 720) {}

	float GetRandFloat()
	{
		return rand() / (float)RAND_MAX;
	}

	float GetRandSignedFloat()
	{
		return (GetRandFloat() - .5) * 2;
	}

	void SetupBone(Bone* rootBone, vec3 localOffset)
	{
		for (size_t i = 0; i < 2; i++)
		{
			rootBone = rootBone->AddChildBone(
				glm::vec3(localOffset),
				glm::quat(1, 0, 0, 0));
		}
	}

	void init() override {
		mousePos = { 0.f, 0.f };
		leftMouseButtonPressed = false;
		altKeyPressed = false;

		additionalShaderData.Pos = { 0.,0.,0. };
		cachedElapsedTime = 0;
		targetPositionOffset = vec3(0, 0, 0);

		targetPositions.push_back(vec3(0));
		targetPositions.push_back(vec3(0));
		targetPositions.push_back(vec3(0));

		std::cout << "Bone length input : " << boneLength;
		rootBone1 = new Bone(vec3(-.7f, spiderHeight, 1.6f), nullptr);
		SetupBone(rootBone1, vec3(-boneLength, 0, .2f));

		rootBone2 = new Bone(vec3(-.7f, spiderHeight, 1.3f), nullptr);
		SetupBone(rootBone2, vec3(-boneLength, 0, .1f));

		rootBone3 = new Bone(vec3(-.7f, spiderHeight, 1.1f), nullptr);
		SetupBone(rootBone3, vec3(-boneLength, 0, -.1f));

		rootBone4 = new Bone(vec3(-.7f, spiderHeight, .9f), nullptr);
		SetupBone(rootBone4, vec3(-boneLength, 0, -.2f));

		rootBone5 = new Bone(vec3(.7f, spiderHeight, 1.6f), nullptr);
		SetupBone(rootBone5, vec3(boneLength, 0, .2f));

		rootBone6 = new Bone(vec3(.7f, spiderHeight, 1.3f), nullptr);
		SetupBone(rootBone6, vec3(boneLength, 0, .1f));

		rootBone7 = new Bone(vec3(.7f, spiderHeight, 1.1f), nullptr);
		SetupBone(rootBone7, vec3(boneLength, 0, -.1f));

		rootBone8 = new Bone(vec3(.7f, spiderHeight, .9f), nullptr);
		SetupBone(rootBone8, vec3(boneLength, 0, -.2f));


		std::cout << "Bone[1] length" << length(rootBone1->GetChildBones()[0]->GetRelativePos()) << std::endl;
	}

	void IterateBackwards(Bone* rootBone, vec3 ikTargetPos)
	{
		targetPositions[targetPositions.size() - 1] = ikTargetPos;

		for (size_t i = targetPositions.size() - 1; i > 0; i--)
		{
			float boneLength = glm::length(rootBone->GetBoneByChainNumber(i, 0)->GetRelativePos());
			vec3 dir = normalize(targetPositions[i - 1] - targetPositions[i]) * boneLength;
			if (i == 2)
			{
				dir.y = max(spiderHeight, dir.y);
				dir = normalize(dir) * boneLength;
			}
			targetPositions[i - 1] = targetPositions[i] + dir;
		}
	}

	void IterateForward(Bone* rootBone)
	{
		targetPositions[0] = rootBone->GetRelativePos();
		for (size_t i = 1; i < targetPositions.size() - 1; i++)
		{
			float boneLength = glm::length(rootBone->GetBoneByChainNumber(i + 1, 0)->GetRelativePos());
			vec3 dir = normalize(targetPositions[i + 1] - targetPositions[i]) * boneLength;
			if (i == 2)
			{
				dir.y = max(spiderHeight, dir.y);
				dir = normalize(dir) * boneLength;
			}
			targetPositions[i + 1] = targetPositions[i] + dir;
		}
	}

	void CalculateNewBonesPosition(Bone* chainRootBone, vec3 ikTargetPos)
	{
		float chainLength = chainRootBone->GetChainLength();

		for (size_t i = 0; i < targetPositions.size(); i++)
		{
			Bone* boneByIndex = chainRootBone->GetBoneByChainNumber(i, 0);
			targetPositions[i] = boneByIndex->GetAbsolutePos();
		}
		//std::cout << "rootBone target pos : " << targetPositions[0].x << "|" << targetPositions[0].y << "|" << targetPositions[0].z << std::endl;

		if (chainLength < length((targetPositions[0] - ikTargetPos)))
		{
			std::cout << "Too long for the chain" << std::endl;
			float currentMaxDistance = 0;
			vec3 dir = normalize(ikTargetPos - chainRootBone->GetAbsolutePos());
			targetPositions[0] = chainRootBone->GetRelativePos();

			//Draw straight
			for (size_t i = 1; i < targetPositions.size(); i++)
			{
				Bone* currentBone = chainRootBone->GetBoneByChainNumber(i, 0);
				currentMaxDistance += length(currentBone->GetRelativePos());
				targetPositions[i] = chainRootBone->GetRelativePos() + dir * currentMaxDistance;
			}
		}

		else
		{
			float targetDistance = length((ikTargetPos - targetPositions[targetPositions.size() - 1]));

			int currentIteration = 0;
			vec3 relativePos = targetPositions[1] - targetPositions[0];
			while (targetDistance > maxDistanceThreshold && currentIteration < maxIterations)
			{

				IterateBackwards(chainRootBone, ikTargetPos);

				IterateForward(chainRootBone);

				currentIteration++;

				if (rootBone1 == chainRootBone)
					std::cout << std::endl;

			}
		}

		chainRootBone->SetRelativePos(targetPositions[0]);

		for (size_t i = 1; i < targetPositions.size(); i++)
		{
			//convert positions to relative
			Bone* boneByIndex = chainRootBone->GetBoneByChainNumber(i, 0);

			if (boneByIndex != nullptr)
			{
				vec3 relativePos = normalize(targetPositions[i] - targetPositions[i - 1]) * length(rootBone1->GetBoneByChainNumber(i, 0)->GetRelativePos());
				//if (length(relativePos) == )
				{
					if (rootBone1 == chainRootBone)
						std::cout << "pos[1] length" << length(relativePos) << std::endl;
					//apply positions
					boneByIndex->SetRelativePos(relativePos);
				}
			}
		}

	}

	void UpdateBoneHeight(Bone* rootBone)
	{
		rootBone->SetRelativePos(vec3(rootBone->GetRelativePos().x, spiderHeight, rootBone->GetRelativePos().z));
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

		targetPosition1 = vec3(0, sin(elapsedTime * walkSpeed) * walkHeight, -cos(elapsedTime * walkSpeed));
		targetPosition1.y = max(0.f, targetPosition1.y);
		targetPosition2 = vec3(0, sin(elapsedTime * walkSpeed + M_PI) * walkHeight, -cos((elapsedTime * walkSpeed) + M_PI));
		targetPosition2.y = max(0.f, targetPosition2.y);

		UpdateBoneHeight(rootBone1);
		UpdateBoneHeight(rootBone2);
		UpdateBoneHeight(rootBone3);
		UpdateBoneHeight(rootBone4);
		UpdateBoneHeight(rootBone5);
		UpdateBoneHeight(rootBone6);
		UpdateBoneHeight(rootBone7);
		UpdateBoneHeight(rootBone8);

		CalculateNewBonesPosition(rootBone1, targetPosition1 + bone1Offset);
		CalculateNewBonesPosition(rootBone2, targetPosition2 + bone2Offset);
		CalculateNewBonesPosition(rootBone3, targetPosition1 + bone3Offset);
		CalculateNewBonesPosition(rootBone4, targetPosition2 + bone4Offset);

		CalculateNewBonesPosition(rootBone5, targetPosition1 + bone5Offset);
		CalculateNewBonesPosition(rootBone6, targetPosition2 + bone6Offset);
		CalculateNewBonesPosition(rootBone7, targetPosition1 + bone7Offset);
		CalculateNewBonesPosition(rootBone8, targetPosition2 + bone8Offset);
	}

	void render3D_custom(const RenderApi3D& api) const override {
		//Here goes your drawcalls affected by the custom vertex shader
		//api.horizontalPlane({ 0, 2, 0 }, { 4, 4 }, 200, vec4(0.0f, 0.2f, 1.f, 1.f));
	}

	void RenderBoneRecursive(const RenderApi3D& api, Bone* boneToRender, vec3 parentBoneAbsPos, quat parentBoneAbsRot, bool skipThisBone = false) const
	{
		vec3 currentBoneAbsPos = boneToRender->GetAbsolutePos(parentBoneAbsPos, parentBoneAbsRot);
		quat currentBoneAbsRot = boneToRender->GetAbsoluteRot(parentBoneAbsRot);

		if (!skipThisBone)
		{
			api.bone(boneToRender->GetRelativePos(), spiderColor, parentBoneAbsRot, parentBoneAbsPos);
		}


		for each (Bone * childBone in boneToRender->GetChildBones())
		{
			RenderBoneRecursive(api, childBone, currentBoneAbsPos, currentBoneAbsRot);
		}
	}

	void render3D(const RenderApi3D& api) const override
	{
		if (targetPosSpheres)
		{
			api.solidSphere(targetPosition1, .2f, 15, 15, blue);
			api.solidSphere(targetPosition2, .2f, 15, 15, red);
		}

		//render spider body
		api.solidSphere(vec3(0, spiderHeight, 0), 1, 30, 30, spiderColor);
		api.solidSphere(vec3(0, spiderHeight, 1.25f), .8f, 30, 30, spiderColor);

		RenderBoneRecursive(api, rootBone1, vec3(0, 0, 0), quat(1, 0, 0, 0), true);
		RenderBoneRecursive(api, rootBone2, vec3(0, 0, 0), quat(1, 0, 0, 0), true);
		RenderBoneRecursive(api, rootBone3, vec3(0, 0, 0), quat(1, 0, 0, 0), true);
		RenderBoneRecursive(api, rootBone4, vec3(0, 0, 0), quat(1, 0, 0, 0), true);
		RenderBoneRecursive(api, rootBone5, vec3(0, 0, 0), quat(1, 0, 0, 0), true);
		RenderBoneRecursive(api, rootBone6, vec3(0, 0, 0), quat(1, 0, 0, 0), true);
		RenderBoneRecursive(api, rootBone7, vec3(0, 0, 0), quat(1, 0, 0, 0), true);
		RenderBoneRecursive(api, rootBone8, vec3(0, 0, 0), quat(1, 0, 0, 0), true);
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

		ImGui::Begin("Spider");

		ImGui::Checkbox("Show demo window", &showDemoWindow);

		ImGui::ColorEdit4("Background color", (float*)&backgroundColor, ImGuiColorEditFlags_NoInputs);
		ImGui::ColorEdit4("Spider color", (float*)&spiderColor, ImGuiColorEditFlags_NoInputs);

		ImGui::DragInt("Max iterations", &maxIterations, -10.f, 10.f);
		ImGui::DragFloat("Max Distance Threshold", &maxDistanceThreshold, -10.f, 10.f);
		ImGui::DragFloat("WalkSpeed", &walkSpeed, -10.f, 10.f);
		ImGui::DragFloat("walkHeight", &walkHeight, -10.f, 10.f);
		ImGui::DragFloat("SpiderHeight", &spiderHeight, -10.f, 10.f);
		ImGui::Checkbox("Target Spheres", &targetPosSpheres);
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		ImGui::End();

		if (showDemoWindow) {
			// Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
			ImGui::ShowDemoWindow(&showDemoWindow);
		}
	}
};
