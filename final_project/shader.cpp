#include "shader.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/glew.h>
#endif

// Constructor: compile shaders, link into program, glGetUniformLocation
Shader::Shader(const std::string& vert_path, const std::string& frag_path, 
			   const std::vector<std::string>& textureNames, int outputs) {
	this->outputCount = outputs;
	std::string vProg, fProg;
	vProg = readShaderFile(vert_path);
	fProg = readShaderFile(frag_path);
	this->program = buildShaders(vProg.c_str(), fProg.c_str());
	this->modelUniform = glGetUniformLocation(this->program, "model");

	for (size_t i = 0; i < textureNames.size(); i++) {
		this->textureUniforms.push_back(glGetUniformLocation(this->program, textureNames[i].c_str()));
	}
}

// Destructor: Clean up shader program
Shader::~Shader() {
	if (this->program != 0) {
		glDeleteProgram(this->program);
	}
}

// Helper function to read shader files
std::string Shader::readShaderFile(std::string shader_path) {
	std::string prog;

	// Load shader into string
	std::string line;
	std::ifstream vert_file{ shader_path };
	if (vert_file)
	{
		while (std::getline(vert_file, line))
		{
			prog += line + "\n";
		}
	}
	return prog;
}

// Helper function to compile shaders and link them into program
GLuint Shader::buildShaders(const char* vertShader, const char* fragShader) {
	GLuint v = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(v, 1, &vertShader, NULL);

	glCompileShader(v);
	// check for shader compile errors
	int success;
	char infoLog[512];
	glGetShaderiv(v, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(v, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
		return 0;
	}

	GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(f, 1, &fragShader, NULL);

	glCompileShader(f);
	// check for shader compile errors
	glGetShaderiv(f, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(f, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
		return 0;
	}

	GLuint p = glCreateProgram();
	glAttachShader(p, v);
	glAttachShader(p, f);
	glLinkProgram(p);

	glGetProgramiv(p, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(p, 512, NULL, infoLog);
		std::cout << "linking failed: " << infoLog << std::endl;
		return 0;
	}

	glDeleteShader(v);
	glDeleteShader(f);

	return p;
}

void Shader::setVec3Array(const std::string& name, GLsizei count, const float* value) const {
	this->use();
	GLint loc = glGetUniformLocation(this->program, name.c_str());
	if (loc != -1) {
		glUniform3fv(loc, count, value);
	}
}

void Shader::setInt(const std::string& name, int value) const {
	this->use();
	GLint loc = glGetUniformLocation(this->program, name.c_str());
	if (loc != -1) {
		glUniform1i(loc, value);
	}
}

void Shader::use() const {
	glUseProgram(this->program);
}

void Shader::draw(const Mesh& mesh, const glm::mat4& model) {
	// Bind program and mesh
	this->use();
	glBindVertexArray(mesh.getVAO());

	if (this->modelUniform != -1) {
		glUniformMatrix4fv(this->modelUniform, 1, GL_FALSE, glm::value_ptr(model));
	}

	// Bind textures (only as many as this shader has samplers)
	for (GLint i = 0; i < std::min(mesh.getTextures().size(), textureUniforms.size()); ++i) {
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, (mesh.getTextures())[i]);
		glUniform1i(textureUniforms[i], i);
	}

	glDrawArrays(GL_TRIANGLES, 0, mesh.getVertexCount());
}

void Shader::drawScreen(const Mesh& quadMesh, const std::vector<GLuint>& gBufferTextures) {
	// Bind program and mesh
	this->use();
	glBindVertexArray(quadMesh.getVAO());

	if (gBufferTextures.size() > textureUniforms.size()) {
		throw std::runtime_error("gBuffer has more textures than the shader has samplers");
	}
	// Bind GBuffer textures
	for (GLint i = 0; i < gBufferTextures.size(); ++i) {
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, gBufferTextures[i]);
		glUniform1i(this->textureUniforms[i], i);
	}

	glDrawArrays(GL_TRIANGLES, 0, quadMesh.getVertexCount());
}