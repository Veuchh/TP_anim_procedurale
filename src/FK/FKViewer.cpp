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
#include "Bone.h"

struct FKViewer : Viewer {

	glm::vec2 mousePos;

	bool leftMouseButtonPressed;
	bool altKeyPressed;
	double cachedElapsedTime;
	glm::vec3 relativeRot;
	VertexShaderAdditionalData additionalShaderData;
	float boneAngle;
	Bone* rootBone;

	FKViewer() : Viewer("FKViewer", 1280, 720) {}

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
		relativeRot = glm::vec3(0, 0, 0);
		rootBone = new Bone(glm::quat(1, 0, 0, 0), glm::vec3(1, 0, 0));
		Bone* currentParentBone = rootBone;
		float defaultOffset = currentParentBone->GetRelativePos().x;
		float currentOffset = currentParentBone->GetRelativePos().x;
		for (size_t i = 0; i < 5; i++)
		{
			currentOffset += defaultOffset;
			currentParentBone = currentParentBone->AddChildBone(
				glm::vec3(currentOffset, 0, 0),
				glm::quat(1, 0, 0, 0));

			if (i == 2)
			{
				currentParentBone->AddChildBone(
					glm::vec3(currentOffset, 3, 0),
					glm::quat(1, 0, 0, 0));
			}

			if (i == 3)
			{
				currentParentBone->AddChildBone(
					glm::vec3(currentOffset, 3, 0),
					glm::quat(1, 0, 0, 0));
				Bone* newBone = currentParentBone->AddChildBone(
					glm::vec3(0, -2, 1),
					glm::quat(1, 0, 0, 0));

				newBone->AddChildBone(
					glm::vec3(0, -2, 1),
					glm::quat(1, 0, 0, 0));

				newBone->AddChildBone(
					glm::vec3(0, -1, 3),
					glm::quat(1, 0, 0, 0));
			}
		}
	}

	void RecursiveSetRelativeRot(glm::vec3 newRelativeRot, Bone* bone) {
		bone->SetRelativeRot(newRelativeRot);

		for each (Bone * childBone in bone->GetChildBones())
		{
			RecursiveSetRelativeRot(newRelativeRot, childBone);
		}
	}


	void update(double elapsedTime) override {

		boneAngle = (float)elapsedTime;
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

		//RecursiveSetRelativeRot(relativeRot, rootBone);
	}

	void render3D_custom(const RenderApi3D& api) const override {
		//Here goes your drawcalls affected by the custom vertex shader
		//api.horizontalPlane({ 0, 2, 0 }, { 4, 4 }, 200, glm::vec4(0.0f, 0.2f, 1.f, 1.f));
	}

	void RenderBoneRecursive(const RenderApi3D& api, Bone* boneToRender, glm::vec3 parentBoneAbsPos, glm::quat parentBoneAbsRot) const
	{
		glm::vec3 currentBoneAbsPos = boneToRender->GetAbsolutePos(parentBoneAbsPos, parentBoneAbsRot);
		glm::quat currentBoneAbsRot = boneToRender->GetAbsoluteRot(parentBoneAbsRot);


		api.bone(boneToRender->GetRelativePos(), white, parentBoneAbsRot, parentBoneAbsPos);

		for each (Bone * childBone in boneToRender->GetChildBones())
		{
			RenderBoneRecursive(api, childBone, currentBoneAbsPos, currentBoneAbsRot);
		}
	}

	void render3D(const RenderApi3D& api) const override
	{
		RenderBoneRecursive(api, rootBone, glm::vec3(0, 0, 0), glm::quat(1, 0, 0, 0));
		std::cout << std::endl;
		std::cout << std::endl;
		std::cout << std::endl;
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
			const glm::vec2 min = mousePos + glm::vec2(padding, padding);
			const glm::vec2 max = mousePos + glm::vec2(-padding, -padding);
			if (leftMouseButtonPressed) {
				api.quadFill(min, max, white);
			}
			else {
				api.quadContour(min, max, white);
			}
		}
	}

	void RecursiveResetRot(Bone* bone) 
	{
		bone->SetRelativeRot(glm::vec3(0, 0, 0));
		for each (Bone * childBone in bone->GetChildBones())
		{
			RecursiveResetRot(childBone);
		}
	}

	void RecursiveDisplayBoneDebug(Bone* bone) {

		if (bone->GetChildBones().size() == 0)
		{
			return;
		}
		glm::vec3 boneRelativeRotEuler = bone->relativeRotEuler;

		ImGui::PushID(bone);
		ImGui::SliderFloat3("RelativeRot", reinterpret_cast<float(&)[3]>(boneRelativeRotEuler), -3.14f, 3.14f);
		ImGui::PopID();
		bone->SetRelativeRot(boneRelativeRotEuler);

		for each (Bone * childBone in bone->GetChildBones())
		{
			RecursiveDisplayBoneDebug(childBone);
		}
	}

	void drawGUI() override {
		static bool showDemoWindow = false;

		ImGui::Begin("FK");

		ImGui::Checkbox("Show demo window", &showDemoWindow);

		ImGui::ColorEdit4("Background color", (float*)&backgroundColor, ImGuiColorEditFlags_NoInputs);
		if (ImGui::Button("Reset Rotations"))
		{
			RecursiveResetRot(rootBone);
		}
		RecursiveDisplayBoneDebug(rootBone);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		ImGui::End();

		if (showDemoWindow) {
			// Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
			ImGui::ShowDemoWindow(&showDemoWindow);
		}
	}
};
