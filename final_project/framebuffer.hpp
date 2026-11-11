#pragma once

#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/glew.h>
#endif
#include <vector>

class Framebuffer {
public:
	Framebuffer(int count, int X, int Y, bool useRBO = true);
	~Framebuffer();

	void bind() const;
	void clear(float r = 0.0f, float g = 0.0f, float b = 0.0f) const;

	GLuint getFBO() const {
		return fbo;
	};
	const std::vector<GLuint>& getTextures() const {
		return textures;
	};
private: 
	GLuint fbo, rbo;
	int width, height;
	std::vector<GLuint> textures;
};