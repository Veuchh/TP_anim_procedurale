#pragma once


#include <algorithm>

#include "../viewer.h"
#include "../drawbuffer.h"
#include "../renderapi.h"

#include <imgui.h>
#include <ostream>
#include <vector>
#include <GLFW/glfw3.h>
#include "../MyViewer.cpp"


struct BoidsViewer : Viewer {

	class Boid {
	public:
		Boid() = default;
		~Boid() = default;
		glm::vec2 position = { 0, 0 };
		glm::vec2 velocity = { 0, 0 };
	};

	glm::vec3 jointPosition;
	glm::vec3 cubePosition;
	float boneAngle;

	glm::vec2 mousePos;

	bool leftMouseButtonPressed;
	bool altKeyPressed;

	VertexShaderAdditionalData additionalShaderData;

	std::vector<Boid*> boids = std::vector<Boid*>();

	// Boids Parameters
	float boidsCoherence = 0.5f;
	float boidsSeparation = 0.5f;
	float boidsAlignment = 0.5f;

	int numBoids = 250;
	float boidsSpeed = 1.6;
	float boidsSpeedLimit = 2.3;
	float boidsModelArrowThickness = 8;
	float boidsModelArrowHat = 77;
	float boidsVisualRange = 36;
	bool mouseAttractBoids = false;
	int maxNeighborForColor = 5;
	glm::vec4 minNeighborColor = { 0.f, 1.f, 0.f, 1.f };
	glm::vec4 maxNeighborColor = { 1.f, 0.f, 0.f, 1.f };

	BoidsViewer() : Viewer("BoidsViewer", 1280, 720) {}

	~BoidsViewer() {
		for (const Boid* boid : boids) {
			delete boid;
		}
	}

	void init() override {
		cubePosition = glm::vec3(1.f, 0.25f, -1.f);
		jointPosition = glm::vec3(-1.f, 2.f, -1.f);
		boneAngle = 0.f;
		mousePos = { 0.f, 0.f };
		leftMouseButtonPressed = false;

		altKeyPressed = false;

		additionalShaderData.Pos = { 0.,0.,0. };
		initBoids();
	}

	void initBoids() {
		for (int i = 0; i < numBoids; i++) {
			Boid* boid = new Boid();
			boid->position = { randFloat()*static_cast<float>(viewportWidth), randFloat()*static_cast<float>(viewportHeight) };
			boid->velocity = { randFloat()*10-5, randFloat()*10-5 };
			boids.push_back(boid);
		}
	}

	static float randFloat() {
		return rand() / static_cast<float>(RAND_MAX);
	}

	static double distance(const Boid& boid1, const Boid& boid2) {
		return glm::distance(boid1.position, boid2.position);
	}

	void keepWithinBounds(Boid& boid) {
		const double margin = 50;
		const double turnFactor = 1;

		if (boid.position.x < margin) {
			boid.velocity.x += turnFactor;
		}
		if (boid.position.x > viewportWidth - margin) {
			boid.velocity.x -= turnFactor;
		}
		if (boid.position.y < margin) {
			boid.velocity.y += turnFactor;
		}
		if (boid.position.y > viewportHeight - margin) {
			boid.velocity.y -= turnFactor;
		}
	}

	void flyTowardCenter(Boid& boid) {
		const float centeringFactor = 0.01f * boidsCoherence; // adjust velocity by this %

		float centerX = 0;
		float centerY = 0;

		int numNeighbors = 0;

		for (const Boid* otherBoid : boids) {
			if (distance(boid, *otherBoid) < boidsVisualRange) {
				centerX += otherBoid->position.x;
				centerY += otherBoid->position.y;
				numNeighbors += 1;
			}
		}

		if (numNeighbors) {
			centerX /= static_cast<float>(numNeighbors);
			centerY /= static_cast<float>(numNeighbors);

			boid.velocity.x += (centerX - boid.position.x) * centeringFactor;
			boid.velocity.y += (centerY - boid.position.y) * centeringFactor;
		}
	}

	void avoidOthers(Boid& boid) const {
		float avoidFactor = 0.1f * boidsSeparation; // Adjust velocity by this %
	    float moveX = 0.0f;
	    float moveY = 0.0f;
	    for (const Boid* otherBoid : boids) {
	        if (otherBoid != &boid) {
		        constexpr float minDistance = 20.0f;
		        if (distance(boid, *otherBoid) < minDistance) {
	                moveX += boid.position.x - otherBoid->position.x;
	                moveY += boid.position.y - otherBoid->position.y;
	            }
	        }
	    }

	    boid.velocity.x += moveX * avoidFactor;
	    boid.velocity.y += moveY * avoidFactor;
	}

	void matchVelocity(Boid& boid) const {
		float avgDX = 0;
		float avgDY = 0;
		int numNeighbors = 0;

		for (const Boid* otherBoid : boids) {
			if (distance(boid, *otherBoid) < boidsVisualRange) {
				avgDX += otherBoid->velocity.x;
				avgDY += otherBoid->velocity.y;
				numNeighbors += 1;
			}
		}

		if (numNeighbors) {
			const float matchingFactor = 0.1f * boidsAlignment;
			avgDX /= static_cast<float>(numNeighbors);
			avgDY /= static_cast<float>(numNeighbors);

			boid.velocity.x += (avgDX - boid.velocity.x) * matchingFactor;
			boid.velocity.y += (avgDY - boid.velocity.y) * matchingFactor;
		}
	}

	void limitSpeed(Boid &boid) const {
		const float speed = glm::length(boid.velocity);
		if (speed > boidsSpeedLimit) {
			boid.velocity = (boid.velocity / speed) * boidsSpeedLimit;
		}
	}

	glm::vec4 getColor(const Boid &boid) const {
		// Count the numbers of neighbors
		int numNeighbors = 0;
		for (const Boid* otherBoid : boids) {
			if (distance(boid, *otherBoid) < boidsVisualRange) {
				numNeighbors += 1;
			}
		}

		// Interpolate color based on number of neighbors
		const float t = std::min(static_cast<float>(numNeighbors) / static_cast<float>(maxNeighborForColor), 1.f);
		return glm::mix(minNeighborColor, maxNeighborColor, t);
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
		for (Boid* boid: boids) {
			flyTowardCenter(*boid);
			avoidOthers(*boid);
			matchVelocity(*boid);
			limitSpeed(*boid);
			keepWithinBounds(*boid);
			boid->position += boid->velocity * boidsSpeed;
		}
	}

	void render3D_custom(const RenderApi3D& api) const override {
	}

	void render3D(const RenderApi3D& api) const override {
	}

	void render2D(const RenderApi2D& api) const override {
		for (Boid* boid: boids) {
			//api.circleFill(boid->position, 5, 10, red);
			api.arrow(boid->position, boid->position + normalize(boid->velocity),boidsModelArrowThickness,boidsModelArrowHat,getColor(*boid));
		}
	}

	void drawGUI() override {
		static bool showDemoWindow = false;

		ImGui::Begin("3D Sandbox - Boids");
		ImGui::SliderFloat("Boids Coherence", &boidsCoherence, 0.0f, 1.0f);
		ImGui::SliderFloat("Boids Separation", &boidsSeparation, 0.0f, 1.0f);
		ImGui::SliderFloat("Boids Alignment", &boidsAlignment, 0.0f, 1.0f);
		ImGui::Separator();
		ImGui::SliderFloat("Boids Speed", &boidsSpeed, 0.0f, 100.0f);
		ImGui::SliderFloat("Boids Speed Limit", &boidsSpeedLimit, boidsSpeed, 100.0f);
		ImGui::SliderFloat("Boids Visual Range", &boidsVisualRange, 0.0f, 100.0f);
		ImGui::SliderFloat("Boids Model Arrow Thickness", &boidsModelArrowThickness, 0.0f, 100.0f);
		ImGui::SliderFloat("Boids Model Arrow Hat", &boidsModelArrowHat, 0.0f, 100.0f);
		ImGui::SliderInt("Boids Neighbor For Color", &maxNeighborForColor, 0, numBoids);
		ImGui::Checkbox("Mouse Attracts Boids", &mouseAttractBoids);

		if (ImGui::CollapsingHeader("Boids Colors")) {
			ImGui::ColorPicker3("Min Neighbors Color", reinterpret_cast<float *>(&minNeighborColor));
			ImGui::ColorPicker3("Max Neighbors Color", reinterpret_cast<float *>(&maxNeighborColor));
		}

		// Drop down
		if (ImGui::CollapsingHeader("Boids List")) {
			// Iterate through boids with index loop
			for (int i = 0; i < numBoids; i++) {
				// Write the current boid index
				ImGui::Text("Boid %d", i);
				// Write position and velocity as text
				ImGui::Text("Position: (%.2f, %.2f)", boids[i]->position.x, boids[i]->position.y);
				ImGui::Text("Velocity: (%.2f, %.2f)", boids[i]->velocity.x, boids[i]->velocity.y);
				ImGui::Separator();
			}
		}

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
			ImGui::SliderFloat3("CustomShader_Pos", &additionalShaderData.Pos.x, -10.f, 10.f);
			ImGui::Separator();
			float fovDegrees = glm::degrees(camera.fov);
			if (ImGui::SliderFloat("Camera field of fiew (degrees)", &fovDegrees, 15, 180)) {
				camera.fov = glm::radians(fovDegrees);
			}

			ImGui::SliderFloat3("Cube Position", (float(&)[3])cubePosition, -1.f, 1.f);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		}

		ImGui::End();

		if (showDemoWindow) {
			// Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
			ImGui::ShowDemoWindow(&showDemoWindow);
		}
	}
};
