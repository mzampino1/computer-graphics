#pragma once

#include <string>
#include <vector>
#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/glew.h>
#endif

class Cubemap {
public:
	// Faces in order: +X, -X, +Y, -Y, +Z, -Z
	Cubemap(const std::vector<std::string>& faces);
	~Cubemap();

	GLuint getTexture() const { return texture; }

private:
	GLuint texture{ 0 };
};