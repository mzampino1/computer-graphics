#include "shader.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/glew.h>
#endif

// Constructor: compile shaders, link into program, glGetUniformLocation
Shader::Shader(const std::string& vert_path, const std::string& frag_path) {
	std::string vProg, fProg;
	vProg = readShaderFile(vert_path);
	fProg = readShaderFile(frag_path);
	this->program = buildShaders(vProg.c_str(), fProg.c_str());
	this->modelUniform = glGetUniformLocation(this->program, "model");
	this->lightDirUniform = glGetUniformLocation(this->program, "lightDir");
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

void Shader::draw(const Mesh& mesh, const glm::mat4& model, const glm::vec3& lightDir) {
	// Bind program and mesh
	glUseProgram(this->program);
	glBindVertexArray(mesh.getVAO());

	if (this->modelUniform != -1) {
		glUniformMatrix4fv(this->modelUniform, 1, GL_FALSE, glm::value_ptr(model));
	}

	if (this->lightDirUniform != -1) {
		glUniform3fv(this->lightDirUniform, 1, glm::value_ptr(lightDir));
	}

	glDrawArrays(GL_TRIANGLES, 0, mesh.getVertexCount());
}