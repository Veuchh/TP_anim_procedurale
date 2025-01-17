#pragma once

#include "../viewer.h"
#include "../drawbuffer.h"
#include "../renderapi.h"

#include <time.h>
#include <imgui.h>
#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vector>
#include <iostream>
#include <tuple>



#include "../MyViewer.cpp"
#include "../renderengine.h"

constexpr int MAX_IMPACT_DATA = 10;

struct ImpactShaderData {
	glm::vec4 Impact; // x = ImpactPosition.x, y = ImpactPosition.y, z = ImpactPosition.z, w = jiggleRadius
	glm::vec4 Infos; // x = timeOfImpact, y = 0, z = 0, w = 0
};

// We use vector 4 instead of vector 3 to respect 16 byte alignment
struct BounceShaderData{
	glm::vec4 Pos; // x = ObjectPosition.x, y =  ObjectPosition.y, z = ObjectPosition.z, w = timeOfImpact
	glm::vec4 Impact; // x = ImpactPosition.x, y = ImpactPosition.y, z = ImpactPosition.z, w = jiggleRadius
	ImpactShaderData ImpactDatas[MAX_IMPACT_DATA];
	/// beware of alignement (std430 rule)
};

struct BounceViewer : Viewer {

	glm::vec3 jointPosition;
	glm::vec3 cubePosition;
	float boneAngle;

	glm::vec2 mousePos;

	bool leftMouseButtonPressed;
	bool rightMouseButtonPressedPrevious;
	bool rightMouseButtonPressed;
	bool rightMouseButtonReleased;
	bool altKeyPressed;
	float elapsedTimeGlobal;

	// Bounce variables
	std::vector<std::tuple<glm::vec3, glm::vec3>> lastRays = std::vector<std::tuple<glm::vec3, glm::vec3>>();
	int lastImpactDataIndex = 0; // To keep track of the last impact data index (kind of circular buffer)
	int impactDataSize = MAX_IMPACT_DATA; // Number of impact data to store (to know for the circular buffer)

	// Bounce Parameters
	float bounceRadius = 1.0f;
	glm::vec3 impactPosition = { 0.f, 0.f, 0.f };
	bool showRays = true;

	BounceShaderData bounceShaderData;

	BounceViewer() : Viewer("BounceViewer", 1280, 720) {}

	void init() override {
		cubePosition = glm::vec3(1.f, 0.25f, -1.f);
		jointPosition = glm::vec3(-1.f, 2.f, -1.f);
		boneAngle = 0.f;
		mousePos = { 0.f, 0.f };
		leftMouseButtonPressed = false;
		elapsedTimeGlobal = 0.f;

		altKeyPressed = false;

		initShaderData(0.0f);
	}

	void initShaderData(float timeOfImpact) {
		bounceShaderData.Pos = { 0.,0.,0.,timeOfImpact}; // center:  ( center.x,   center.y,   center.z,   timeOfImpact )
		bounceShaderData.Impact = { impactPosition, bounceRadius}; // impact:  ( impact.x,   impact.y,   impact.z,   jiggleRadius )
	}


	void update(double elapsedTime) override {
		boneAngle = static_cast<float>(elapsedTime);
		elapsedTimeGlobal = static_cast<float>(elapsedTime);
		leftMouseButtonPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
		rightMouseButtonPressedPrevious = rightMouseButtonPressed;
		rightMouseButtonPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
		if (rightMouseButtonPressed && !rightMouseButtonPressedPrevious) {
            rightMouseButtonReleased = true;
        } else {
            rightMouseButtonReleased = false;
        }

		altKeyPressed = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;

		double mouseX;
		double mouseY;
		glfwGetCursorPos(window, &mouseX, &mouseY);

		// Only bounce on right click
		if (rightMouseButtonReleased) {
            bounceOnClick();
        }

		mousePos = { float(mouseX), viewportHeight - float(mouseY) };

		pCustomShaderData = &bounceShaderData;
		CustomShaderDataSize = sizeof(BounceShaderData);

		// Print the entire bounceShaderData
		// std::cout << "bounceShaderData.Pos: " << bounceShaderData.Pos.x << ", " << bounceShaderData.Pos.y << ", " << bounceShaderData.Pos.z << ", " << bounceShaderData.Pos.w << std::endl;
		// for (int i = 0; i < MAX_IMPACT_DATA; ++i) {
		// 	std::cout << "bounceShaderData.ImpactDatas[" << i << "].Impact: " << bounceShaderData.ImpactDatas[i].Impact.x << ", " << bounceShaderData.ImpactDatas[i].Impact.y << ", " << bounceShaderData.ImpactDatas[i].Impact.z << ", " << bounceShaderData.ImpactDatas[i].Impact.w << std::endl;
		// 	std::cout << "bounceShaderData.ImpactDatas[" << i << "].Infos: " << bounceShaderData.ImpactDatas[i].Infos.x << ", " << bounceShaderData.ImpactDatas[i].Infos.y << ", " << bounceShaderData.ImpactDatas[i].Infos.z << ", " << bounceShaderData.ImpactDatas[i].Infos.w << std::endl;
		// }
	}

	void bounceOnClick() {
		glm::vec2 screenSize = {viewportWidth, viewportHeight};
		glm::vec2 mouseClipPose =  (2.f * mousePos / screenSize) - glm::vec2(1.f, 1.f);
		glm::mat4 projection = glm::perspective(camera.fov, static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight), 0.1f, 100.f);
		glm::mat4 view = glm::lookAt(glm::vec3(0,0,0), camera.o - camera.eye, camera.up);
		glm::mat4 projectionView = projection * view;
		projectionView = glm::inverse(projectionView);
		glm::vec4 mouseWorldPose = projectionView * glm::vec4(mouseClipPose, 1.f, 1.f);
		glm::vec3 rayCastDirection = glm::vec3(mouseWorldPose);
		glm::vec3 rayOrigin = camera.eye;

		lastRays.emplace_back(camera.eye, rayCastDirection);

	    // --- Plane intersection math starts here ---
	    // We consider a horizontal plane at y=0 with normal (0,1,0)
	    glm::vec3 planeNormal(0.f, 1.f, 0.f);

	    float denom = glm::dot(planeNormal, rayCastDirection);

	    // Avoid division by zero if rayCastDirection is parallel to plane
	    if (std::fabs(denom) > 1e-6f) {
	        // "t" is the distance along rayCastDirection from rayOrigin
	        float t = -glm::dot(planeNormal, rayOrigin) / denom;

	        if (t >= 0.0f) {
	            // The intersection/impact point in world space
	            glm::vec3 impact = rayOrigin + t * rayCastDirection;

	            // If you need to ensure the impact is within the 5×5 region:
	            //  e.g., check if |impact.x| <= 2.5 && |impact.z| <= 2.5

	            // Use or store 'impact' as needed
	            // Check the last impact data index
	        	// This is a circular buffer
	        	int nextImpactDataIndex = (lastImpactDataIndex + 1) % impactDataSize;
	        	bounceShaderData.ImpactDatas[nextImpactDataIndex].Impact = { impact, bounceRadius };
	        	bounceShaderData.ImpactDatas[nextImpactDataIndex].Infos = { elapsedTimeGlobal, 0.f, 0.f, 0.f };
	        	lastImpactDataIndex = nextImpactDataIndex;

	            std::cout << "Impact at: "
	                      << impact.x << ", "
	                      << impact.y << ", "
	                      << impact.z << std::endl;
	        }
	        else {
	            // Intersection lies behind the camera
	        }
	    }
	    else {
	        // Ray is parallel to the plane
	    }
	}


	void render3D_custom(const RenderApi3D& api) const override {
		//Here goes your drawcalls affected by the custom vertex shader
		api.horizontalPlane({ 0, 0, 0 }, { 5, 5 }, 200, glm::vec4(0.0f, 0.2f, 1.f, 1.f));
		//api.solidCube(0.5f,red,nullptr);
	}

	void render3D(const RenderApi3D& api) const override {
		if (showRays) {
			for (std::tuple<glm::vec3, glm::vec3> ray: lastRays) {
				const glm::vec3& origin = std::get<0>(ray);
				const glm::vec3& dir = std::get<1>(ray);
				const glm::vec3 end = origin + dir * 100.f;
				const glm::vec3 vertices[] = { origin, end };
				api.lines(vertices, 2, red, nullptr);
			}
		}
	}

	void render2D(const RenderApi2D& api) const override {

		constexpr float padding = 50.f;

		if (altKeyPressed) {
			if (leftMouseButtonPressed) {
				api.circleFill(mousePos, padding, 10, white);
			} else {
				api.circleContour(mousePos, padding, 10, white);
			}

		} else {
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

	void drawGUI() override {
		static bool showDemoWindow = false;
		ImGui::Begin("3D Sandbox - BounceViewer");
		ImGui::SliderFloat("Bounce Radius", &bounceRadius, 0.1f, 10.f);
		ImGui::SliderFloat3("Impact Position", &impactPosition.x, -5.f, 5.f);
		if (ImGui::Button("Refresh Shaders Data")) {
			initShaderData(elapsedTimeGlobal);
			lastRays.clear();
        }
		ImGui::Checkbox("Show Rays", &showRays);

		if (ImGui::CollapsingHeader("Default")) {
			ImGui::Checkbox("Show demo window", &showDemoWindow);

			ImGui::ColorEdit4("Background color", (float*)&backgroundColor, ImGuiColorEditFlags_NoInputs);

			ImGui::SliderFloat("Point size", &pointSize, 0.1f, 10.f);
			ImGui::SliderFloat("Line Width", &lineWidth, 0.1f, 10.f);
			ImGui::Separator();
			ImGui::SliderFloat3("Light dir", (float(&)[3])lightDir, -1.f, 1.f);
			ImGui::SliderFloat("Light Strength", &lightStrength, 0.f, 2.f);
			ImGui::SliderFloat("Ligh Ambient", &lightAmbient, 0.f, 0.5f);
			ImGui::SliderFloat("Ligh Specular", &specular, 0.f, 1.f);
			ImGui::SliderFloat("Ligh Specular Pow", &specularPow, 1.f, 200.f);
			ImGui::Separator();
			ImGui::SliderFloat3("CustomShader_Pos", &bounceShaderData.Pos.x, -10.f, 10.f);
			ImGui::Separator();
			float fovDegrees = glm::degrees(camera.fov);
			if (ImGui::SliderFloat("Camera field of fiew (degrees)", &fovDegrees, 15, 180)) {
				camera.fov = glm::radians(fovDegrees);
			}

			ImGui::SliderFloat3("Cube Position", (float(&)[3])cubePosition, -1.f, 1.f);
		}

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		ImGui::End();

		if (showDemoWindow) {
			// Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
			ImGui::ShowDemoWindow(&showDemoWindow);
		}
	}
};

