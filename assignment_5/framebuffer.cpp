#include "framebuffer.hpp"
#include <iostream>
#include <stdexcept>

Framebuffer::Framebuffer(int count, int X, int Y, bool useRBO) 
		: fbo{ 0 }, rbo{ 0 } {
	if (count < 1 || count > 8) {
		throw std::invalid_argument("Framebuffer: count must be 1-8");
	}
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	textures.resize(count);
	glGenTextures(count, &textures[0]);

	for (int i = 0; i < count; i++) {
		glBindTexture(GL_TEXTURE_2D, textures[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, X, Y, 0, GL_RGBA, GL_FLOAT, 0);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, textures[i], 0);
	}

	GLenum colorAttach[] = {
		GL_COLOR_ATTACHMENT0,
		GL_COLOR_ATTACHMENT0 + 1,
		GL_COLOR_ATTACHMENT0 + 2,
		GL_COLOR_ATTACHMENT0 + 3,
		GL_COLOR_ATTACHMENT0 + 4,
		GL_COLOR_ATTACHMENT0 + 5,
		GL_COLOR_ATTACHMENT0 + 6,
		GL_COLOR_ATTACHMENT0 + 7
	};

	glDrawBuffers(count, colorAttach);

	if (useRBO) {
		glGenRenderbuffers(1, &rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, X, Y);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
	}

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		// Clean up
		if (rbo != 0) glDeleteRenderbuffers(1, &rbo);
		if (!textures.empty()) glDeleteTextures(static_cast<GLsizei>(textures.size()), &textures[0]);
		if (fbo != 0) glDeleteFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		throw std::runtime_error("Framebuffer creation failed");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Framebuffer::~Framebuffer() {
	if (rbo != 0) glDeleteRenderbuffers(1, &rbo);
	if (!textures.empty()) glDeleteTextures((GLsizei)(textures.size()), &textures[0]);
	if (fbo != 0) glDeleteFramebuffers(1, &fbo);
}

void Framebuffer::bind() const {
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

void Framebuffer::clear(float r, float g, float b) const {
	glClearColor(r, g, b, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}