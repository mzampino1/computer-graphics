#include "cubemap.hpp"
#include <SDL3/SDL.h>
#include <stdexcept>

Cubemap::Cubemap(const std::vector<std::string>& faces) {
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

	for (GLuint i = 0; i < faces.size(); i++) {
		SDL_Surface* face = SDL_LoadBMP(faces[i].c_str());
		if (face == nullptr) throw std::runtime_error("Failed to load " + faces[i]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, face->w, face->h, 0,
			GL_BGR, GL_UNSIGNED_BYTE, face->pixels);
		SDL_DestroySurface(face);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

Cubemap::~Cubemap() {
	glDeleteTextures(1, &texture);
}