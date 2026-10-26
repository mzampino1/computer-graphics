#include <iostream>
#include <cstdint>
#include <SDL3/SDL.h>
#include "glm/glm.hpp"
#include "ThreadPool.hpp"

constexpr int RESX = 960;
constexpr int RESY = 540;

void clearSurface(SDL_Surface*);
void drawPixel(SDL_Surface*, size_t, size_t, const glm::ivec3&);
void drawTriangle(std::mutex&, size_t, size_t, void*);
struct Triangle
{
	SDL_Surface* draw;
	glm::vec3 positions[3];
	glm::ivec3 color;
};

int main(int argc, char** argv)
{
	ThreadsPool::pool();
	SDL_Window* window = nullptr;

	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		std::cerr << "Failed to init SDL: " << SDL_GetError() << '\n';
		return -1;
	}

	window = SDL_CreateWindow("Hello Window", RESX, RESY, 0);

	if (!window)
	{
		std::cerr << "Failed to create a window: " << SDL_GetError() << '\n';
		return -2;
	}

	SDL_Surface* buffer = SDL_CreateSurface(RESX, RESY, SDL_PIXELFORMAT_RGBA8888);
	SDL_Event event;
	bool end = false;
	while (!end)
	{
		while (SDL_PollEvent(&event))
		{
			switch(event.type)
			{
			case SDL_EVENT_QUIT: end = true; break;
			case SDL_EVENT_KEY_DOWN: std::cout << "Key pressed\n"; break;
			case SDL_EVENT_KEY_UP: std::cout << "Key released\n"; break;
			}
		}
		clearSurface(buffer);
		
		Triangle tri;
		tri.draw = buffer;
		tri.color = {64, 224, 208};
		tri.positions[0] = glm::vec3{ 200.0f, 400.0f, 0.0f };
		tri.positions[1] = glm::vec3{ 400.0f, 100.0f, 0.0f };
		tri.positions[2] = glm::vec3{ 760.0f, 470.0f, 0.0f };

		ThreadsPool::pool().dispatch(buffer->w * buffer->h, drawTriangle, &tri, false);

		SDL_BlitSurface(buffer, NULL, SDL_GetWindowSurface(window), NULL);
		SDL_UpdateWindowSurface(window);
	}

	SDL_DestroySurface(buffer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

void clearSurface(SDL_Surface* buf)
{
	std::uint32_t* pixels = static_cast<std::uint32_t*>(buf->pixels);
	for (int i = 0; i < buf->w; ++i)
	{
		for (int j = 0; j < buf->h; ++j)
		{
			pixels[j * buf->w + i] = SDL_MapSurfaceRGBA(buf, 0xFF, 0xFF, 0xFF, 0xFF);
		}
	}
}

void drawTriangle(std::mutex& m, size_t start, size_t end, void* data) {
	Triangle* tri = static_cast<Triangle*>(data);
	for (size_t i = start; i < end; ++i)
	{
		size_t x = i % tri->draw->w;
		size_t y = i / tri->draw->w;

		glm::vec3 a = tri->positions[0];
		glm::vec3 b = tri->positions[1];
		glm::vec3 c = tri->positions[2];
		// Compute cross products
		float cp_ab = (a[0] - x) * (b[1] - y) - (a[1] - y) * (b[0] - x);
		float cp_bc = (b[0] - x) * (c[1] - y) - (b[1] - y) * (c[0] - x);
		float cp_ca = (c[0] - x) * (a[1] - y) - (c[1] - y) * (a[0] - x);

		// Binary Predicate: Check if point is inside the triangle (all cross
		// products have the same sign)
		if (cp_ab >= 0.0f and cp_bc >= 0.0f and cp_ca >= 0.0f or
			cp_ab < 0.0f and cp_bc < 0.0f and cp_ca < 0.0f) {
			drawPixel(tri->draw, x, y, tri->color);
		}
	}
}

void drawPixel(SDL_Surface* buff, size_t x, size_t y, const glm::ivec3& color) {
	// Make sure pixel is within window
	if (x >= buff->w)
	{
		return;
	}
	if (y >= buff->h)
	{
		return;
	}

	std::uint32_t* pixels = static_cast<std::uint32_t*>(buff->pixels);
	pixels[y * buff->w + x] = SDL_MapSurfaceRGBA(buff, color.x, color.y, color.z, 0xFF);
}