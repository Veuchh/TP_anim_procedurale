#version 430 core

#define BufferAttribVertex 0
#define BufferAttribNormal 1
#define BufferAttribColor 2

const float impactDurationInSeconds = 2.0;

// You can compil and refresh the shader at runtime using the F7 key

//-- Uniform are variable that are common to all vertices of the drawcall
uniform mat4 Model; // Model matrix
uniform mat4 View;  // View matrix
uniform mat4 Projection; // Projection Matrix
uniform float Time; // Elapsed time since the beginning of the program

layout(location = BufferAttribVertex) in vec3 Position;
layout(location = BufferAttribNormal) in vec3 Normal;
layout(location = BufferAttribColor) in vec4 Color;

//--------------------------------------------------
// Data structures & buffer
//--------------------------------------------------
struct ImpactShaderData {
	vec4 Impact; // x = ImpactPosition.x, y = ImpactPosition.y, z = ImpactPosition.z, w = jiggleRadius
	vec4 Infos;  // x = timeOfImpact, y = 0, z = 0, w = 0
};

layout(std430, binding=3) buffer bufferData
{
	vec4 center; // center: ( center.x, center.y, center.z, timeOfImpact )
	vec4 impact; // impact: ( impact.x, impact.y, impact.z, jiggleRadius )  [Unused now]
	ImpactShaderData impactDatas[10]; // impactDatas[10]: array of impacts actually in use
} Data;

//--------------------------------------------------
// Outputs to fragment shader
//--------------------------------------------------
out block
{
	vec4 Color;
	vec3 CameraSpacePosition;
	vec3 CameraSpaceNormal;
} Out;

//--------------------------------------------------
// Main
//--------------------------------------------------
void main()
{
	//------------------------------------------------
	// 1) Compute new vertex position with center offset
	//------------------------------------------------
	mat4 MV = View * Model;
	vec4 newPos = vec4(Position, 1.0);

	// Apply center offset
	newPos.x += Data.center.x;
	newPos.y += Data.center.y;
	newPos.z += Data.center.z;

	//------------------------------------------------
	// 2) Compute cumulative effect of all impacts in impactDatas[10]
	//------------------------------------------------
	float finalGradient = 0.0;  // Tracks the maximum gradient
	float finalMaxY     = 0.0;  // Tracks the vertical displacement from that max impact

	// Colors used for the gradient effect
	vec4 blackColor = vec4(0.0, 0.0, 0.0, 1.0);
	vec4 whiteColor = vec4(1.0, 1.0, 1.0, 1.0);

	// Loop through all impacts
	for (int i = 0; i < 10; i++)
	{
		//------------------------------------------------
		// a) Basic data for this impact
		//------------------------------------------------
		float distanceToImpact_i = length(newPos.xyz - Data.impactDatas[i].Impact.xyz);
		float jiggleRadius_i     = Data.impactDatas[i].Impact.w;

		float timeOfImpact_i     = Data.impactDatas[i].Infos.x;
		float timeSinceImpact_i  = Time - timeOfImpact_i;

		// How far along the impact's "lifetime" we are
		float impactProgress_i = clamp(timeSinceImpact_i / impactDurationInSeconds, 0.0, 1.0);

		//------------------------------------------------
		// b) Compute local gradient for this impact
		//------------------------------------------------
		float distanceFactor_i = distanceToImpact_i / jiggleRadius_i;    // 0..∞
		float gradient_i       = clamp(1.0 - distanceFactor_i, 0.0, 1.0);

		// Ease-out quadratic
		gradient_i = 1.0 - ((1.0 - gradient_i) * (1.0 - gradient_i));

		// Fade out as impact finishes
		gradient_i *= (1.0 - impactProgress_i);

		//------------------------------------------------
		// c) "Jiggle" vertical displacement
		//------------------------------------------------
		//   Avoid dividing by 0 if timeSinceImpact_i = 0
		float maxY_i = 0.0;
		if (timeSinceImpact_i != 0.0) {
			maxY_i = sin(timeSinceImpact_i * 25.0) * (1.0 / timeSinceImpact_i);
		}

		//------------------------------------------------
		// d) Keep the "strongest" impact
		//------------------------------------------------
		if (gradient_i > finalGradient) {
			finalGradient = gradient_i;
			finalMaxY     = maxY_i;
		}
	}

	//------------------------------------------------
	// 3) Apply final gradient & displacement
	//------------------------------------------------
	newPos.y += finalGradient * finalMaxY;

	vec4 finalColor = mix(blackColor, whiteColor, finalGradient);

	//------------------------------------------------
	// 4) Fill the "Out" data
	//------------------------------------------------
	Out.CameraSpacePosition = vec3(MV * newPos);
	Out.CameraSpaceNormal   = vec3(MV * vec4(Normal, 0.0));
	Out.Color               = finalColor;

	//------------------------------------------------
	// 5) Final vertex position
	//------------------------------------------------
	gl_Position = Projection * MV * newPos;
}
