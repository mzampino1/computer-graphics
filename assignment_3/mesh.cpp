#include "mesh.hpp"

#include <fstream>
#include <iostream>
#include <vector>

Mesh::Mesh(const std::string& filepath) {
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

	// Build VBO/VAO
	glGenVertexArrays(1, &this->vao);
	glGenBuffers(1, &this->vbo);
	glBindVertexArray(this->vao);
	glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * buff.size(), &buff[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(1);
	// Start of normal data
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)(this->vertex_count * 3 * sizeof(float)));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR)
	{
		std::cout << "vao errs present: " << err << '\n';
	}
}

Mesh::~Mesh() {
	glDeleteVertexArrays(1, &this->vao);
	glDeleteBuffers(1, &this->vbo);
}