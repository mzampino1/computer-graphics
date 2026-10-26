#pragma once

#include <string>
#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/glew.h>
#endif

class Mesh {
public:
	// Constructor: Loads exported Blender text file, builds VBO and VAO
	Mesh(const std::string& filepath);

	// Destructor: Deletes VBO and VAO
	~Mesh();

	// Getters
	GLuint getVAO() const { 
		return vao;
	}
	GLuint getVBO() const { 
		return vbo;
	}
	int getVertexCount() const {
		return vertex_count;
	}

private:
	GLuint vbo{ 0 };
	GLuint vao{ 0 };
	int vertex_count{ 0 };
	std::string name{ "" };
};