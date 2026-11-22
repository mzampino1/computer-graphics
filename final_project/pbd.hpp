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

	// Collision elasticity (0 = no bounce, 1 = perfectly elastic)
	float particleRestitution = 0.7f;   // marble, marble
	float groundRestitution = 0.3f;   // marble, ground

	void step(float dt);
private:
	void solveParticleCollisions();
	void solveGroundCollisions();
	// Ground state for the current substep, interpolated between
	// the previous frame and the current frame
	glm::vec3 substepGroundPoint, substepGroundNormal;

	// Collision recorded during a substep, used to add bounce afterward
	struct Contact {
		size_t particle1, particle2;
		bool groundCollision; // true when particle1 hits the ground
		glm::vec3 normal;     // points from particle2 (or the ground) toward particle1
		float vnBefore;       // speed along normal before collision solved (< 0 = approaching)
	};
	std::vector<Contact> contacts;
	void applyRestitution(float subDt);
};
