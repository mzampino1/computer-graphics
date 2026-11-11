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

	void step(float dt);
private:
	void solveParticleCollisions();
	void solveGroundCollisions();
};
