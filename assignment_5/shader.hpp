#pragma once

#include "mesh.hpp"
#include "framebuffer.hpp"

#include <string>
#include <vector>
#include <glm/glm.hpp>
#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/glew.h>
#endif

class Shader {
public:
	// Constructor
	Shader(const std::string& vert_path, const std::string& frag_path,
		const std::vector<std::string>& textureNames = { "albedo", "normal" },
		int outputs = 1);

	// Destructor
	~Shader();

	// Getters
	GLuint getProgram() const {
		return program;
	}

	// Uniform Setters
	void setVec3Array(const std::string& name, GLsizei count, const float* value) const;
	void setInt(const std::string& name, int value) const;

	void use() const;

	bool canDrawTo(const Framebuffer& fb) const {
		return (int)fb.getTextures().size() == outputCount;
	}

	// Draw calls
	void draw(const Mesh& mesh, const glm::mat4& model);
	void drawScreen(const Mesh& quadMesh, const std::vector<GLuint>& gBufferTextures);

private:
	std::string readShaderFile(std::string shader_path);
	GLuint buildShaders(const char* vertShader, const char* fragShader);
	GLuint program{ 0 };
	GLint modelUniform{ -1 };
	GLint lightDirUniform{ -1 };
	std::vector<GLint> textureUniforms;
	int outputCount{ 1 };
};