#pragma once

#include "mesh.hpp"

#include <string>
#include <glm/glm.hpp>
#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/glew.h>
#endif

class Shader {
public:
	// Constructor
	Shader(const std::string& vert_path, const std::string& frag_path);

	// Destructor
	~Shader();

	// Getters
	GLuint getProgram() const {
		return program;
	}

	// Draw mesh to screen
	void draw(const Mesh& mesh, const glm::mat4& model, const glm::vec3& lightDir);

private:
	std::string readShaderFile(std::string shader_path);
	GLuint buildShaders(const char* vertShader, const char* fragShader);
	GLuint program{ 0 };
	GLint modelUniform{ -1 };
	GLint lightDirUniform{ -1 };
};