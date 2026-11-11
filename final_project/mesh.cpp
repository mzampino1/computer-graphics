#include "mesh.hpp"

#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <SDL3/SDL.h>

Mesh::Mesh(const std::string& filepath, const std::vector<std::string>& bitmap_paths) {
	std::ifstream file(filepath);
	if (!file) {
		std::cerr << "Failed to open mesh file: " << filepath << "\n";
		return;
	}
	
	// Read vertex data from file
	std::vector<float> buff;
	file >> this->vertex_count;
	
	for (int i = 0; i < this->vertex_count; i++) {
		float x, y, z;
		file >> x;
		file >> y;
		file >> z;
		buff.push_back(x);
		buff.push_back(y);
		buff.push_back(z);
	}
	for (int i = 0; i < this->vertex_count; i++) {
		float x_n, y_n, z_n;
		file >> x_n;
		file >> y_n;
		file >> z_n;
		buff.push_back(x_n);
		buff.push_back(y_n);
		buff.push_back(z_n);
	}
	for (int i = 0; i < this->vertex_count; i++) {
		float u, v;
		file >> u;
		file >> v;
		buff.push_back(u);
		buff.push_back(v);
	}

	// Calculations for tangents, bitangents

	size_t offset = vertex_count * 6;//offset to UV coords
	std::vector<float> tangents, bitangents;
	for (size_t v = 0, u = offset, i = 0; i < vertex_count; v += 3 * 3, u += 2 * 3, i += 3)
	{
		//Triangle positions
		glm::vec3 a(buff[v], buff[v + 1], buff[v + 2]);
		glm::vec3 b(buff[v + 3], buff[v + 4], buff[v + 5]);
		glm::vec3 c(buff[v + 6], buff[v + 7], buff[v + 8]);
		//UV coordinates
		glm::vec2 uA(buff[u], buff[u + 1]);
		glm::vec2 uB(buff[u + 2], buff[u + 3]);
		glm::vec2 uC(buff[u + 4], buff[u + 5]);
		//edges
		glm::vec3 ab = b - a, ac = c - a;
		glm::vec2 uAB = uB - uA, uAC = uC - uA;
		//Inverse determinant
		float invD = 1.0f / (uAB.x * uAC.y - uAC.x * uAB.y);
		for (int j = 0; j < 3; ++j)//store three times for one triangle, i.e., vertices
		{
			//You can save some compute -- store vars outside the loop :)
			tangents.push_back(invD * (uAC.y * ab.x - uAB.y * ac.x));
			tangents.push_back(invD * (uAC.y * ab.y - uAB.y * ac.y));
			tangents.push_back(invD * (uAC.y * ab.z - uAB.y * ac.z));
			bitangents.push_back(invD * (-uAC.x * ab.x + uAB.x * ac.x));
			bitangents.push_back(invD * (-uAC.x * ab.y + uAB.x * ac.y));
			bitangents.push_back(invD * (-uAC.x * ab.z + uAB.x * ac.z));
		}
	}
	buff.insert(buff.end(), tangents.begin(), tangents.end());
	buff.insert(buff.end(), bitangents.begin(), bitangents.end());

	// Build VBO/VAO
	glGenVertexArrays(1, &this->vao);
	glGenBuffers(1, &this->vbo);
	glBindVertexArray(this->vao);
	glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * buff.size(), &buff[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	// Start of normal data
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)(this->vertex_count * 3 * sizeof(float)));
	// Start of UV data
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)(this->vertex_count * 6 * sizeof(float)));
	// Start of tangent data
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (void*)(this->vertex_count * 8 * sizeof(float)));
	// Start of bitangent data
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, (void*)(this->vertex_count * 11 * sizeof(float)));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR)
	{
		std::cout << "vao errs present: " << err << '\n';
	}

	// Build textures
	if (!bitmap_paths.size()) return;
	textureIDs.resize(bitmap_paths.size());
	glGenTextures((GLsizei)(textureIDs.size()), &textureIDs[0]);
	for (size_t i = 0; i < bitmap_paths.size(); ++i)
	{
		SDL_Surface* bmp = SDL_LoadBMP(bitmap_paths[i].c_str());
		if (!bmp)
		{
			std::cerr << bitmap_paths[i] << " failed to load\n";
			continue;
		}
		glBindTexture(GL_TEXTURE_2D, textureIDs[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, bmp->w, bmp->h, 0, GL_BGR, GL_UNSIGNED_BYTE,
			bmp->pixels);
		glGenerateMipmap(GL_TEXTURE_2D);
		SDL_DestroySurface(bmp);
		glBindTexture(GL_TEXTURE_2D, 0);
		GLenum err;
		while ((err = glGetError()) != GL_NO_ERROR)
		{
			std::cout << "texture errs present: " << err << '\n';
		}
	}
}

Mesh::~Mesh() {
	glDeleteVertexArrays(1, &this->vao);
	glDeleteBuffers(1, &this->vbo);
	if (!textureIDs.empty()) {
		glDeleteTextures((GLsizei)(textureIDs.size()), &textureIDs[0]);
	}
}