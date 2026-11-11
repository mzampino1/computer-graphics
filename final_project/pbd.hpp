#pragma once

#include <vector>
#include <glm/glm.hpp>

struct Particle {
	glm::vec3 pos, prevPos, velocity;
	float radius, invMass;
};

class PBDSolver {
public:
	std::vector<Particle> particles;
	glm::vec3 gravity{ 0.0f, -9.8f, 0.0f };
	int substeps = 10;

	// Ground plane (current frame)
	glm::vec3 groundPoint{ 0.0f, -2.0f, 0.0f };
	glm::vec3 groundNormal{ 0.0f, 1.0f, 0.0f };

	// Ground plane (previous frame)
	glm::vec3 prevGroundPoint{ 0.0f, -2.0f, 0.0f };
	glm::vec3 prevGroundNormal{ 0.0f, 1.0f, 0.0f };

	void step(float dt);
private:
	void solveParticleCollisions();
	void solveGroundCollisions();
	// Ground state for the current substep, interpolated between
	// the previous frame and the current frame
	glm::vec3 substepGroundPoint, substepGroundNormal;
};
