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
#include "../MyViewer.cpp"
#include "ClothParticle.hpp"
#include "ClothConstraint.hpp"
#include <vector>
#include <iostream>
#include <algorithm>
#include <tuple>


struct ClothViewer : Viewer {

	glm::vec3 jointPosition;
	glm::vec3 cubePosition;
	float boneAngle;

	glm::vec2 mousePos;

	bool leftMouseButtonPressed;
	bool rightMouseButtonPressed;
	bool altKeyPressed;

	VertexShaderAdditionalData additionalShaderData;

	// Cloth variables
	std::vector<ClothParticle> particles = std::vector<ClothParticle>();
	std::vector<ClothConstraint> constraints = std::vector<ClothConstraint>();
	std::vector<ClothParticle*> anchorParticles = std::vector<ClothParticle*>();
	std::vector<std::tuple<glm::vec3, glm::vec3>> lastRays = std::vector<std::tuple<glm::vec3, glm::vec3>>();
	float previousElapsedTime = 0.f;
	float deltaTime = 0.f;
	float distanceBetweenParticlesOnSpawn = .5f;
	int clothWidth = 20; // The width of the cloth
	int clothLength = 1; // The length of the cloth
	int clothHeight = 20; // The height of the cloth

	// Cloth Parameters
	int solverIterations = 1;
	int subSteps = 16;
	glm::vec3 gravity = { 0.f, -9.81f, 0.f };
	glm::vec3 wind = { 0.f, 0.f, 0.f };
	float airFriction = 0.5f;
	bool showClothParticles = true;
	bool showClothConstraints = true;
	bool showRays = true;
	float clothConstraintStrength = 1.f;
	float clothConstraintMaxElongationRatio = 1.5f;

	ClothViewer() : Viewer("ClothViewer", 1280, 720) {}

	void init() override {
		cubePosition = glm::vec3(1.f, 0.25f, -1.f);
		jointPosition = glm::vec3(-1.f, 2.f, -1.f);
		boneAngle = 0.f;
		mousePos = { 0.f, 0.f };
		leftMouseButtonPressed = false;

		altKeyPressed = false;

		additionalShaderData.Pos = { 0.,0.,0. };

		initCloth();
	}

	void initCloth() {

		// Clear the particles and constraints (to enable resets)
		particles.clear();
		constraints.clear();
		anchorParticles.clear();
		lastRays.clear();

		// Create particles in a 3D grid
		for (int x = 0; x < clothWidth; ++x) {
			for (int y = 0; y < clothHeight; ++y) {
				for (int z = 0; z < clothLength; ++z) {
					const glm::vec3 position = glm::vec3(x, y, z) * distanceBetweenParticlesOnSpawn;
					ClothParticle particle;
					particle.position = position;
					particles.emplace_back(particle);
				}
			}
		}

		// Create links between particles and their neighbors
		for (int x = 0; x < clothWidth; ++x) {
			for (int y = 0; y < clothHeight; ++y) {
				for (int z = 0; z < clothLength; ++z) {
					const int index = x * clothHeight * clothLength + y * clothLength + z;
					if (x < clothWidth - 1) createLink(particles[index], particles[index + clothHeight * clothLength]);
					if (y < clothHeight - 1) createLink(particles[index], particles[index + clothLength]);
					if (z < clothLength - 1) createLink(particles[index], particles[index + 1]);
				}
			}
		}

		// Make the top layer of cloth particles non-moving
		/*for (int x = 0; x < clothWidth; ++x) {
			for (int z = 0; z < clothLength; ++z) {
				const int index = x * clothHeight * clothLength + (clothHeight - 1) * clothLength + z;
				particles[index].moving = false;
			}
		}*/

		// Make the top layer of cloth particles non-moving, but only the most left and the most right
		for (int z = 0; z < clothLength; ++z) {
			const int index1 = (clothWidth - 1) * clothHeight * clothLength + (clothHeight - 1) * clothLength + z;
			const int index2 = 0 * clothHeight * clothLength + (clothHeight - 1) * clothLength + z;
			particles[index1].moving = false;
			particles[index2].moving = false;
			anchorParticles.emplace_back(&particles[index1]);
			anchorParticles.emplace_back(&particles[index2]);
		}
	}

	void createLink(ClothParticle& particle1, ClothParticle& particle2) {
		ClothConstraint constraint = ClothConstraint(particle1, particle2, clothConstraintStrength, clothConstraintMaxElongationRatio);
		constraints.emplace_back(constraint);
	}

	void update(double elapsedTime) override {
		// Elapsed time is the time since the start of the application
		// deltaTime is the time since the last frame
		deltaTime = static_cast<float>(elapsedTime - previousElapsedTime);
		previousElapsedTime = static_cast<float>(elapsedTime);

		boneAngle = (float)elapsedTime;

		leftMouseButtonPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
		rightMouseButtonPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

		altKeyPressed = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;

		double mouseX;
		double mouseY;
		glfwGetCursorPos(window, &mouseX, &mouseY);

		mousePos = { float(mouseX), viewportHeight - float(mouseY) };

		pCustomShaderData = &additionalShaderData;
		CustomShaderDataSize = sizeof(VertexShaderAdditionalData);

		if (rightMouseButtonPressed) cutLinksUnderMouse();

		const float subStepDeltaTime = deltaTime / static_cast<float>(subSteps);
		removeBrokenLinks();
		for (int i = subSteps; i--;) {
			applyGravity();
			applyWind();
			applyAirFriction();
			updatePositions(subStepDeltaTime);
			solveConstraints();
			updateDerivatives(subStepDeltaTime);
		}
	}

	void removeBrokenLinks() {
		constraints.erase(
			std::remove_if(
				constraints.begin(),
				constraints.end(),
				[](const ClothConstraint& c) {
					return c.broken;
				}
			),
			constraints.end()
		);
	}


	void applyGravity() {
		for (ClothParticle& clothParticle: particles) {
			clothParticle.forces += gravity * clothParticle.mass;
		}
	}

	void applyWind() {
		for (ClothParticle& clothParticle: particles) {
			clothParticle.forces += wind * clothParticle.mass;
		}
	}

	void applyAirFriction() {
		for (ClothParticle& clothParticle: particles) {
			clothParticle.forces -= clothParticle.velocity * airFriction;
		}
	}

	void updatePositions(const float subStepDeltaTime) {
		for (ClothParticle& clothParticle: particles) {
			clothParticle.update(subStepDeltaTime);
		}
	}

	void solveConstraints() {
		for (int i = solverIterations; i--;) {
			for (ClothConstraint& constraint: constraints) {
				constraint.solve();
			}
		}
	}

	void updateDerivatives(const float subStepDeltaTime) {
		for (ClothParticle& clothParticle: particles) {
			clothParticle.updateDerivatives(subStepDeltaTime);
		}
	}


	void cutLinksUnderMouse()
	{
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

		auto distanceRayToSegment = [&](const glm::vec3 &p0, const glm::vec3 &p1)
		{
		    // One line is our RAY:  R(s) = rayOrigin + s * rayCastDirection, s >= 0
		    // The other is our SEGMENT: S(t) = p0 + t * (p1 - p0), t in [0,1]

		    glm::vec3 segDir = p1 - p0;      // segment direction
		    glm::vec3 w0 = rayOrigin - p0;   // vector between p0 and ray origin
		    float segLenSqr = glm::dot(segDir, segDir);

		    // If segment is degenerate, treat it as a point:
		    if (segLenSqr < 1e-12f)
		    {
		        // The best "t" on segment is 0 => p0 itself
		        // We just find the closest point on the ray
		        float s = glm::dot((p0 - rayOrigin), rayCastDirection);
		        if (s < 0.f)
		            s = 0.f;  // behind the camera; clamp to origin

		        glm::vec3 closestOnRay = rayOrigin + s * rayCastDirection;
		        return glm::length(closestOnRay - p0);
		    }

		    // Now for the standard line-vs-line approach:
		    //
		    // Let:
		    //   D = rayCastDirection
		    //   d = segDir
		    //   w0 = (rayOrigin - p0)
		    //
		    // We define the following “dot products”:
		    float a = glm::dot(segDir, segDir);     // = |d|^2
		    float b = glm::dot(segDir, rayCastDirection); // d·D
		    float c = glm::dot(rayCastDirection, rayCastDirection); // |D|^2 (should be 1 if D is normalized, but let's keep it general)
		    float d_ = glm::dot(segDir, w0);        // d·(rayOrigin - p0)
		    float e = glm::dot(rayCastDirection, w0); // D·(rayOrigin - p0)

		    float denom = a*c - b*b;

		    float s, t;

		    // If denom ~ 0, the lines are almost parallel
		    if (fabs(denom) < 1e-12f)
		    {
		        // Force s=0 (choose the ray origin as best approach)
		        s = 0.0f;
		        // Then find t in [0..1] that is closest to R(0) = rayOrigin
		        // t = dot( p0->rayOrigin, segDir ) / |segDir|^2
		        t = d_ / a;
		        t = glm::clamp(t, 0.f, 1.f);
		    }
		    else
		    {
		        // Non-parallel case
		        s = (b*d_ - a*e) / denom;
		        t = (c*d_ - b*e) / denom;

		        // Now clamp t to [0..1] for the segment
		        if (t < 0.f)
		        {
		            t = 0.f;  // front endpoint
		        }
		        else if (t > 1.f)
		        {
		            t = 1.f;  // back endpoint
		        }
		    }

		    // Also clamp s >= 0 for the RAY
		    if (s < 0.f)
		    {
		        s = 0.f;
		    }

		    // Compute the actual points
		    glm::vec3 closestOnRay = rayOrigin + s * rayCastDirection;
		    glm::vec3 closestOnSeg = p0 + t * segDir;

		    // Return the distance
		    return glm::length(closestOnRay - closestOnSeg);
		};


	    // 3) Iterate over every constraint and check distance. If under threshold, break it.
	    const float cutThreshold = .5f;
	    for (ClothConstraint& constraint : constraints)
	    {
	        if (constraint.broken) {
	            continue; // already broken, skip
	        }

	        // Retrieve the segment endpoints
	        glm::vec3 p0 = constraint.particle1.get().position;
	        glm::vec3 p1 = constraint.particle2.get().position;

	        // Compute distance from the ray to the constraint's segment
	        float dist = distanceRayToSegment(p0, p1);
	        if (dist < cutThreshold) {
	            constraint.broken = true;
	        }
	    }
	}



	void render3D_custom(const RenderApi3D& api) const override {
		//Here goes your drawcalls affected by the custom vertex shader
		//api.horizontalPlane({ 0, 2, 0 }, { 4, 4 }, 200, glm::vec4(0.0f, 0.2f, 1.f, 1.f));
	}

	void render3D(const RenderApi3D& api) const override {

		api.grid(10.f, 10, glm::vec4(0.5f, 0.5f, 0.5f, 1.f), nullptr);

		// Render the cloth particles and constraints

		if (showClothParticles) {
			for (const ClothParticle& clothParticle: particles) {
				api.solidSphere(clothParticle.position, 0.1f, 10, 10, white);
			}
		}

		if (showClothConstraints) {
			for (const ClothConstraint& constraint: constraints) {
				const glm::vec3 vertices[] = { constraint.particle1.get().position, constraint.particle2.get().position };
				api.lines(vertices, 2, white, nullptr);
			}
		}

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

		{
			const glm::vec2 from = { viewportWidth * 0.5f, padding };
			const glm::vec2 to = { viewportWidth * 0.5f, 2.f * padding };
			constexpr float thickness = padding * 0.25f;
			constexpr float hatRatio = 0.3f;
			api.arrow(from, to, thickness, hatRatio, white);
		}

		{
			glm::vec2 vertices[] = {
				{ padding, viewportHeight - padding },
				{ viewportWidth * 0.5f, viewportHeight - 2.f * padding },
				{ viewportWidth * 0.5f, viewportHeight - 2.f * padding },
				{ viewportWidth - padding, viewportHeight - padding },
			};
			api.lines(vertices, COUNTOF(vertices), white);
		}
	}

	void drawGUI() override {
		static bool showDemoWindow = false;

		ImGui::Begin("3D Sandbox - Cloth Viewer");
		if (ImGui::Button("Reset Simulation")) {
			initCloth();
		}
		ImGui::SliderFloat3("Gravity", reinterpret_cast<float(&)[3]>(gravity), -10.f, 10.f);
		ImGui::SliderFloat3("Wind", reinterpret_cast<float(&)[3]>(wind), -10.f, 10.f);
		ImGui::SliderFloat("Air Friction", &airFriction, 0.f, 1.f);
		ImGui::Checkbox("Show Cloth Particles", &showClothParticles);
		ImGui::Checkbox("Show Cloth Constraints", &showClothConstraints);
		ImGui::Checkbox("Show Rays", &showRays);


		if (ImGui::CollapsingHeader("Cloth Particles")) {
			for (int i = 0; i < anchorParticles.size(); ++i) {
				ClothParticle* clothParticle = anchorParticles[i];
				ImGui::Text("Anchor Particle %d", i);
				ImGui::Text("Position: (%.2f, %.2f, %.2f)", clothParticle->position.x, clothParticle->position.y, clothParticle->position.z);
				if (!clothParticle->moving) {
					if (ImGui::Button("Remove Attach")) {
						clothParticle->moving = true;
					}
				}
				else {
					if (ImGui::Button("Attach")) {
						clothParticle->moving = false;
					}
				}

			}
			for (int i = 0; i < particles.size(); ++i) {
				const ClothParticle& clothParticle = particles[i];
				ImGui::Text("Particle %d", i);
				ImGui::Text("Position: (%.2f, %.2f, %.2f)", clothParticle.position.x, clothParticle.position.y, clothParticle.position.z);
				ImGui::SliderFloat("Mass", &particles[i].mass, 0.1f, 10.f);
				ImGui::Separator();
			}
		}

		if (ImGui::CollapsingHeader("Cloth Constraints")) {
			ImGui::SliderFloat("Constraint Strength", &clothConstraintStrength, 0.f, 10.f);
			ImGui::SliderFloat("Max Elongation Ratio", &clothConstraintMaxElongationRatio, 1.f, 10.f);
			if (ImGui::Button("Update All Constraints")) {
				for (ClothConstraint& constraint: constraints) {
					constraint.strength = clothConstraintStrength;
					constraint.maxElongationRatio = clothConstraintMaxElongationRatio;
				}
			}

			if (ImGui::Button("Break 5 Random")) {
				// Break 5 random constraints
				for (int i = 0; i < 5; ++i) {
					const int index = rand() % constraints.size();
					constraints[index].broken = true;
				}
			}
			for (int i = 0; i < constraints.size(); ++i) {
				ClothConstraint& clothConstraint = constraints[i];
				if (clothConstraint.broken) break;
				ImGui::Text("Constraint %d", i);
				if (ImGui::Button("Break")) {
					clothConstraint.broken = true;
				}
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
