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
#include "Particle.h"
#include "Well.h"

struct ParticlesViewer : Viewer {

	glm::vec2 mousePos;

	bool leftMouseButtonPressed;
	bool altKeyPressed;
	float cubeSize;
	float wellSize;
	float wellStrength;
	double cachedElapsedTime = 0;

	std::vector<Particle*>particles = std::vector<Particle*>();
	std::vector<Well*>wells = std::vector<Well*>();

	VertexShaderAdditionalData additionalShaderData;

	ParticlesViewer() : Viewer("ParticlesViewer", 1280, 720) {}

	float GetRandFloat()
	{
		return rand() / (float)RAND_MAX;
	}

	float GetRandSignedFloat()
	{
		return (GetRandFloat() - .5) * 2;
	}

	void AddParticle()
	{
		particles.push_back(
			new Particle(
				glm::vec3(GetRandSignedFloat() * cubeSize / 2, cubeSize * GetRandFloat(), GetRandSignedFloat() * cubeSize / 2),
				glm::vec3(GetRandSignedFloat() * .01, GetRandSignedFloat() * .01, GetRandSignedFloat() * .01)));
	}

	void AddWell()
	{
		wells.push_back(
			new Well(
				glm::vec3(GetRandSignedFloat() * cubeSize / 2, cubeSize * GetRandFloat(), GetRandSignedFloat() * cubeSize / 2),
				wellSize));
	}

	glm::vec3 GetParticleExternalForce(Particle* particle) 
	{
		glm::vec3 externalForce = glm::vec3();
		for each (Well* well  in wells)
		{
			externalForce += well->GetPullVectorFromPosition(particle->GetPosition());
		}

		return glm::vec3(externalForce.x * wellStrength, externalForce.y * wellStrength, externalForce.z * wellStrength);;
	}

	void init() override {
		mousePos = { 0.f, 0.f };
		leftMouseButtonPressed = false;
		cubeSize = 10;
		altKeyPressed = false;
		wellSize = 2;
		wellStrength = 1;

		additionalShaderData.Pos = { 0.,0.,0. };
		double cachedElapsedTime = 0;
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

		//apply particle forces
		for each (Particle * particle in particles)
		{
			particle->SetNewPositionFromForce(cubeSize, GetParticleExternalForce(particle), deltaTime);
		}
	}

	void render3D_custom(const RenderApi3D& api) const override {
		//Here goes your drawcalls affected by the custom vertex shader
		//api.horizontalPlane({ 0, 2, 0 }, { 4, 4 }, 200, glm::vec4(0.0f, 0.2f, 1.f, 1.f));
	}


	void render3D(const RenderApi3D& api) const override {

		glm::vec3 vertices[24] =
		{
			//Bottom sqare
			glm::vec3(-cubeSize / 2, 0 ,-cubeSize / 2),
			glm::vec3(-cubeSize / 2, 0 ,cubeSize / 2),

			glm::vec3(-cubeSize / 2, 0 ,cubeSize / 2),
			glm::vec3(cubeSize / 2, 0 ,cubeSize / 2),

			glm::vec3(cubeSize / 2, 0 ,cubeSize / 2),
			glm::vec3(cubeSize / 2, 0,-cubeSize / 2),

			glm::vec3(cubeSize / 2, 0 ,-cubeSize / 2),
			glm::vec3(-cubeSize / 2, 0 ,-cubeSize / 2),

			//Bottom to top
			glm::vec3(-cubeSize / 2, 0 ,-cubeSize / 2),
			glm::vec3(-cubeSize / 2, cubeSize ,-cubeSize / 2),

			glm::vec3(-cubeSize / 2, 0 ,cubeSize / 2),
			glm::vec3(-cubeSize / 2, cubeSize ,cubeSize / 2),

			glm::vec3(cubeSize / 2, 0 ,cubeSize / 2),
			glm::vec3(cubeSize / 2, cubeSize ,cubeSize / 2),

			glm::vec3(cubeSize / 2, 0 ,-cubeSize / 2),
			glm::vec3(cubeSize / 2, cubeSize ,-cubeSize / 2),

			//Top Square

			glm::vec3(-cubeSize / 2, cubeSize ,-cubeSize / 2),
			glm::vec3(-cubeSize / 2, cubeSize ,cubeSize / 2),

			glm::vec3(-cubeSize / 2, cubeSize ,cubeSize / 2),
			glm::vec3(cubeSize / 2, cubeSize ,cubeSize / 2),

			glm::vec3(cubeSize / 2, cubeSize ,cubeSize / 2),
			glm::vec3(cubeSize / 2, cubeSize,-cubeSize / 2),

			glm::vec3(cubeSize / 2, cubeSize ,-cubeSize / 2),
			glm::vec3(-cubeSize / 2, cubeSize ,-cubeSize / 2),
		};
		api.lines(vertices, 24, glm::vec4(0.5f, 0.5f, 0.5f, 1.f), nullptr);

		//render particles
		for each (Particle * particle in particles)
		{
			api.solidSphere(particle->GetPosition(), .1f, 3, 3, red);
		}

		//Render wells
		for each (Well * well in wells)
		{
			api.solidSphere(well->GetPosition(), well->GetSize(), 10, 10, glm::vec4(0,0,.3,.3));
		}
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

	void drawGUI() override {
		static bool showDemoWindow = false;

		ImGui::Begin("Particles");

		ImGui::Checkbox("Show demo window", &showDemoWindow);

		ImGui::ColorEdit4("Background color", (float*)&backgroundColor, ImGuiColorEditFlags_NoInputs);
		ImGui::DragFloat("CubeSize", (float*)&cubeSize);

		if (ImGui::Button("Spawn Particle")) 
		{
			AddParticle();
		}

		if (ImGui::Button("Spawn 10 Particles"))
		{
			for (size_t i = 0; i < 10; i++)
			{

				AddParticle();
			}
		}

		if (ImGui::Button("Spawn 100 Particles")) 
		{
			for (size_t i = 0; i < 100; i++)
			{

				AddParticle();
			}
		}
		ImGui::DragFloat("wellSize", (float*)&wellSize, 0, 3);
		ImGui::SliderFloat("wellStrength", (float*)&wellStrength, 0, 20);

		if (ImGui::Button("Add Well")) 
		{
			AddWell();
		}

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		ImGui::End();

		if (showDemoWindow) {
			// Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
			ImGui::ShowDemoWindow(&showDemoWindow);
		}
	}
};
