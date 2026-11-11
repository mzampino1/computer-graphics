#include "pbd.hpp"
#include <cmath>

void PBDSolver::step(float dt) {
	float subDt = dt / substeps;

	for (int s = 0; s < substeps; s++) {
		// Partially tilt ground
		float t = float(s + 1) / substeps;
		substepGroundPoint = glm::mix(prevGroundPoint, groundPoint, t);
		substepGroundNormal = glm::normalize(glm::mix(prevGroundNormal, groundNormal, t));

		// Apply gravity, move to a new predicted position
		for (size_t i = 0; i < particles.size(); i++) {
			Particle& p = particles[i];
			if (p.invMass == 0.0f) continue; // particle is pinned
			p.prevPos = p.pos;
			p.velocity += gravity * subDt;
			p.pos += p.velocity * subDt;
		}

		// Fix overlaps caused by prediction
		solveParticleCollisions();
		solveGroundCollisions();

		// Update velocity based on how far each particle moved
		for (size_t i = 0; i < particles.size(); i++) {
			Particle& p = particles[i];
			if (p.invMass == 0.0f) continue;
			p.velocity = (p.pos - p.prevPos) / subDt;
		}
	}
	prevGroundPoint = groundPoint;
	prevGroundNormal = groundNormal;
}

void PBDSolver::solveParticleCollisions() {
	for (size_t i = 0; i < particles.size(); i++) {
		for (size_t j = i + 1; j < particles.size(); j++) {
			Particle& a = particles[i];
			Particle& b = particles[j];

			float invMassSum = a.invMass + b.invMass;
			if (invMassSum == 0.0f) continue; // both pinned

			glm::vec3 delta = a.pos - b.pos;
			float dist = glm::length(delta);
			float minDist = a.radius + b.radius;
			if (dist >= minDist || dist < 1e-6f) continue; // not touching, or overlapping

			// Push particles apart along the line between their centroids,
			// splitting push based on invMass
			glm::vec3 collisionNorm = delta / dist;
			float overlap = minDist - dist;
			a.pos += collisionNorm * overlap * (a.invMass / invMassSum);
			b.pos -= collisionNorm * overlap * (b.invMass / invMassSum);
		}
	}
}

void PBDSolver::solveGroundCollisions() {
	for (size_t i = 0; i < particles.size(); i++) {
		Particle& p = particles[i];
		if (p.invMass == 0.0f) continue; // pinned

		// Signed distance from plane to surface of particle
		float d = glm::dot(p.pos - substepGroundPoint, substepGroundNormal) - p.radius;
		if (d < 0.0f) {
			p.pos -= d * groundNormal;
		}
	}
}
