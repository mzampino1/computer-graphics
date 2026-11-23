#include "pbd.hpp"
#include <cmath>
#include <fstream>
#include <iostream>

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
		contacts.clear();
		solveParticleCollisions();
		solveGroundCollisions();
		solveTriangleCollisions();

		// Update velocity based on how far each particle moved
		for (size_t i = 0; i < particles.size(); i++) {
			Particle& p = particles[i];
			if (p.invMass == 0.0f) continue;
			p.velocity = (p.pos - p.prevPos) / subDt;
		}

		applyRestitution(subDt);
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
			if (dist >= minDist || dist < 1e-6f) continue; // not touching, or exactly on top of each other

			// Push particles apart along the line between their centroids,
			// splitting push based on invMass
			glm::vec3 collisionNorm = delta / dist;

			// Record contact for restitution
			float vnBefore = glm::dot(a.velocity - b.velocity, collisionNorm);
			contacts.push_back({ i, j, false, collisionNorm, vnBefore });

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
			float vnBefore = glm::dot(p.velocity, substepGroundNormal);
			contacts.push_back({ i, 0, true, substepGroundNormal, vnBefore });

			p.pos -= d * substepGroundNormal;
		}
	}
}

void PBDSolver::applyRestitution(float subDt) {
	// Slow contacts get no bounce
	float threshold = 2.0f * glm::length(gravity) * subDt;

	for (size_t c = 0; c < contacts.size(); c++) {
		const Contact& con = contacts[c];
		if (-con.vnBefore <= threshold) continue;

		if (con.staticCollision) {
			Particle& p = particles[con.particle1];
			float vnNow = glm::dot(p.velocity, con.normal);
			float vnTarget = -groundRestitution * con.vnBefore;
			p.velocity += con.normal * (vnTarget - vnNow);
		}
		else {
			Particle& a = particles[con.particle1];
			Particle& b = particles[con.particle2];
			float invMassSum = a.invMass + b.invMass;
			if (invMassSum == 0.0f) continue;

			float vnNow = glm::dot(a.velocity - b.velocity, con.normal);
			float vnTarget = -particleRestitution * con.vnBefore;
			float dv = vnTarget - vnNow;
			a.velocity += con.normal * dv * (a.invMass / invMassSum);
			b.velocity -= con.normal * dv * (b.invMass / invMassSum);
		}
	}
}

bool PBDSolver::addTriangleMesh(const std::string& path, const glm::mat4& model) {
	std::ifstream file(path);
	if (!file) {
		std::cerr << "Failed to open collision mesh: " << path << "\n";
		return false;
	}

	int count;
	file >> count;
	std::vector<glm::vec3> pos(count);
	for (int i = 0; i < count; i++) {
		float x, y, z;
		file >> x >> y >> z;
		pos[i] = glm::vec3(model * glm::vec4(x, y, z, 1.0f));
	}

	for (int i = 0; i + 2 < count; i += 3) {
		Triangle t;
		t.a = pos[i]; t.b = pos[i + 1]; t.c = pos[i + 2];

		// Skip zero-area triangles
		if (glm::length(glm::cross(t.b - t.a, t.c - t.a)) < 1e-8f) continue;

		t.center = (t.a + t.b + t.c) / 3.0f;
		t.boundRadius = glm::max(glm::length(t.a - t.center),
			glm::max(glm::length(t.b - t.center), glm::length(t.c - t.center)));
		triangles.push_back(t);

		int index = (int)triangles.size() - 1;

		// Smallest and largest corner of the triangle's bounding box
		glm::vec3 lo = glm::min(t.a, glm::min(t.b, t.c));
		glm::vec3 hi = glm::max(t.a, glm::max(t.b, t.c));

		// Which cubes those corners fall in
		glm::ivec3 cubeLo = glm::ivec3(glm::floor(lo / cellSize));
		glm::ivec3 cubeHi = glm::ivec3(glm::floor(hi / cellSize));

		// List the triangle in every cube between them
		for (int x = cubeLo.x; x <= cubeHi.x; x++)
			for (int y = cubeLo.y; y <= cubeHi.y; y++)
				for (int z = cubeLo.z; z <= cubeHi.z; z++)
					grid[{x, y, z}].push_back(index);
	}
	return true;
}

static glm::vec3 closestPointOnTriangle(const glm::vec3& p, const glm::vec3& a,
	const glm::vec3& b, const glm::vec3& c) {
	glm::vec3 ab = b - a, ac = c - a, ap = p - a;
	float d1 = glm::dot(ab, ap), d2 = glm::dot(ac, ap);
	if (d1 <= 0.0f && d2 <= 0.0f) return a;                       // corner a

	glm::vec3 bp = p - b;
	float d3 = glm::dot(ab, bp), d4 = glm::dot(ac, bp);
	if (d3 >= 0.0f && d4 <= d3) return b;                         // corner b

	float vc = d1 * d4 - d3 * d2;
	if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)                  // edge ab
		return a + ab * (d1 / (d1 - d3));

	glm::vec3 cp = p - c;
	float d5 = glm::dot(ab, cp), d6 = glm::dot(ac, cp);
	if (d6 >= 0.0f && d5 <= d6) return c;                         // corner c

	float vb = d5 * d2 - d1 * d6;
	if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)                   // edge ac
		return a + ac * (d2 / (d2 - d6));

	float va = d3 * d6 - d5 * d4;
	if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)     // edge bc
		return b + (c - b) * ((d4 - d3) / ((d4 - d3) + (d5 - d6)));

	float denom = 1.0f / (va + vb + vc);                          // inside the face
	return a + ab * (vb * denom) + ac * (vc * denom);
}

void PBDSolver::solveTriangleCollisions() {
	for (size_t i = 0; i < particles.size(); i++) {
		Particle& p = particles[i];
		if (p.invMass == 0.0f) continue;

		glm::ivec3 cubeLo = glm::ivec3(glm::floor((p.pos - p.radius) / cellSize));
		glm::ivec3 cubeHi = glm::ivec3(glm::floor((p.pos + p.radius) / cellSize));

		for (int x = cubeLo.x; x <= cubeHi.x; x++)
		for (int y = cubeLo.y; y <= cubeHi.y; y++)
		for (int z = cubeLo.z; z <= cubeHi.z; z++) {
			auto found = grid.find({ x, y, z });
			if (found == grid.end()) continue;      // nothing in this cube

			const std::vector<int>& nearby = found->second;
			for (size_t k = 0; k < nearby.size(); k++) {
				int t = nearby[k];
				const Triangle& tri = triangles[t];

				// marble can't reach this triangle
				float reach = tri.boundRadius + p.radius;
				glm::vec3 toCenter = p.pos - tri.center;
				if (glm::dot(toCenter, toCenter) > reach * reach) continue;

				glm::vec3 closest = closestPointOnTriangle(p.pos, tri.a, tri.b, tri.c);
				glm::vec3 delta = p.pos - closest;
				float dist = glm::length(delta);
				if (dist >= p.radius) continue; // not touching

				// Push away from the closest point
				glm::vec3 n = (dist > 1e-6f) ? delta / dist
					: glm::normalize(glm::cross(tri.b - tri.a, tri.c - tri.a));

				// Record contact for restitution (treated like the ground)
				Contact con;
				con.particle1 = i;
				con.particle2 = 0;
				con.staticCollision = true;
				con.normal = n;
				con.vnBefore = glm::dot(p.velocity, n);
				contacts.push_back(con);

				p.pos += n * (p.radius - dist);
			}
		}
	}	
}