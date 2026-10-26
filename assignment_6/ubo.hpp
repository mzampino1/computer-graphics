#pragma once

#include <cstddef>
#include <string>
#include <iostream>
#include <stdexcept>
#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/glew.h>
#endif

template <typename T, size_t N>
class UBO {
	static_assert(sizeof(T) % 16 == 0, "UBO element type must be a multiple of 16 bytes");
public:
	UBO(GLuint bindingPoint) 
		: buffer{ 0 }, binding{ bindingPoint }, data{} {
		// Allocate GPU memory for N elements or type T
		glGenBuffers(1, &buffer);
		glBindBuffer(GL_UNIFORM_BUFFER, buffer);
		glBufferData(GL_UNIFORM_BUFFER, N * sizeof(T), data, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}
	~UBO() {
		if (buffer != 0) glDeleteBuffers(1, &buffer);
	}
	void updateData(const T& changes, size_t index) {

		// Modify one element of UBO
		if (index >= N) {
			throw std::out_of_range("UBO::updateData: index out of range");
		}
		data[index] = changes;

		glBindBuffer(GL_UNIFORM_BUFFER, buffer);
		glBufferSubData(GL_UNIFORM_BUFFER, index * sizeof(T), sizeof(T), &data[index]);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}
	void linkToProgram(GLuint program, const std::string& blockName) {
		// Attach this buffer to a binding point, then point the program's block at it
		glBindBufferBase(GL_UNIFORM_BUFFER, this->binding, buffer);

		GLuint blockIndex = glGetUniformBlockIndex(program, blockName.c_str());
		if (blockIndex == GL_INVALID_INDEX) {
			std::cerr << "Uniform block not found: " << blockName << '\n';
			return;
		}
		glUniformBlockBinding(program, blockIndex, this->binding);
	}
private:
	GLuint buffer;
	GLuint binding;
	T data[N];
};