#pragma once

#include <vector>
#include <string>
#include <map>
#include <tuple>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct Particle {
	glm::vec3 pos, prevPos, velocity;
	float radius, invMass;
	glm::quat orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // current rotation
	glm::vec3 angularVelocity = glm::vec3(0.0f);
};

struct Triangle {
	glm::vec3 a, b, c; // world space
	glm::vec3 center;
	float boundRadius;
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

	float rollingResistance = 0.05f; // fraction of speed lost per second

	std::vector<Triangle> triangles;

	// Load a mesh's triangles from a .txt file
	bool addTriangleMesh(const std::string& path, const glm::mat4& model);
	void step(float dt);
private:
	void solveParticleCollisions();
	void solveTriangleCollisions();
	void solveGroundCollisions();
	// Ground state for the current substep, interpolated between
	// the previous frame and the current frame
	glm::vec3 substepGroundPoint, substepGroundNormal;

	// Collision recorded during a substep, used to add bounce afterward
	struct Contact {
		size_t particle1, particle2;
		bool staticCollision; // true when particle1 hits ground or track
		glm::vec3 normal;     // points from particle2 (or the ground) toward particle1
		float vnBefore;       // speed along normal before collision solved (< 0 = approaching)
	};
	std::vector<Contact> contacts;

	// Grid of 0.5 m cubes 
	// Key = cube coordinates (x, y, z), value = triangles in that cube
	float cellSize = 0.5f;
	std::map<std::tuple<int, int, int>, std::vector<int>> grid;

	void applyRestitution(float subDt);

	void updateRotation(float subDt);
};
