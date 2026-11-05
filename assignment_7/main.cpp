#include "mesh.hpp"
#include "shader.hpp"
#include "framebuffer.hpp"
#include "ubo.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <cmath>
#include <SDL3/SDL.h>
#include <fstream>
#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/glew.h>
#endif
#include <SDL3/SDL_opengl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

constexpr int RESX = 1920;
constexpr int RESY = 1080;

struct RenderObject {
	Mesh* mesh;
	glm::mat4 model;
};

void drawCircle(const glm::ivec2& mouseCoords, float radius, SDL_Surface* surface, bool add);
void drawPixel(SDL_Surface* buff, size_t x, size_t y, const glm::ivec3& color);

int main(int argc, char** argv)
{
	//Window is created dynamically, so we use a window pointer
	SDL_Window* window = nullptr;
	SDL_Window* heightmapWindow = nullptr;
	SDL_GLContext context;//This is our instance of OpenGL, it will be bound to a window

	//This is just us initializing SDL with features we want
	//	In this case, just video support
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		std::cerr << "Failed to initialize SDL: " << SDL_GetError();
		return 1;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	//If no failure occurred, then we can build the window
	//	The last argument are boolean flags we will append to in the future
	window = SDL_CreateWindow("Your Very Own Window :)", RESX, RESY, SDL_WINDOW_OPENGL | SDL_WINDOW_INPUT_FOCUS);
	heightmapWindow = SDL_CreateWindow("Heightmap", 1024, 1024, 0);
	if (window == nullptr or heightmapWindow == nullptr)
	{
		std::cerr << "Failed to create window: " << SDL_GetError();
		SDL_Quit();
		return 2;
	}
	context = SDL_GL_CreateContext(window);
#ifdef __APPLE__
	//Not much going on, but the else below is for more than one OS :)
#else
	glewExperimental = true;
	GLenum glewErr = glewInit();
	if (glewErr != GLEW_OK)
	{
		std::cerr << "Glew error: " << glewGetErrorString(glewErr);
		SDL_GL_DestroyContext(context);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 3;
	}
#endif

	glEnable(GL_DEPTH_TEST);
	glClearDepth(1.0f);
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);

	// This code in block so that mesh and shader destructors run before
	// GL_DestroyContext
	{

		// Load meshes, textures, and shaders
		Mesh plane("Plane.txt", { "heightmap.bmp", "atlas.bmp" });

		std::vector<RenderObject> scene = {
			{ &plane, glm::mat4(1.0f) }
		};

		Mesh screen("Screen.txt", {});

		Shader terrain("terrain.vert", "terrain.frag", "terrain.geom", { "height", "atlas" }, 3);
		Shader shadow("shadow.vert", "shadow.frag", "", { "height" }, 1);
		std::vector<std::string> texture_names = { "color", "gbuff_normal", "position", "shadowMap" };
		Shader lighting("light.vert", "light.frag", "", texture_names);

		// Construct view and projection matrices
		glm::mat4 view = glm::lookAt(
			glm::vec3(0.0f, 1.4f, 1.9f),   // Position
			glm::vec3(0.0f, 0.1f, 0.0f),   // Looking at
			glm::vec3(0.0f, 1.0f, 0.0f)    // Up vector
		);
		glm::mat4 projection = glm::perspective(
			glm::radians(45.0f),
			static_cast<float>(RESX) / static_cast<float>(RESY),
			0.1f,
			200.0f
		);

		// Construct lightView and ortho matrices
		glm::vec3 lightDir = glm::vec3(-1.0f, -0.8f, -0.4f);
		glm::mat4 lightView = glm::lookAt(
			glm::vec3(5.0f, 4.0f, 1.0f),
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 1.0f, 0.0f)
		);
		glm::mat4 lightOrtho = glm::ortho(
			-1.6f, 1.6f,   // left, right
			-1.6f, 1.6f,   // bottom, top
			0.1f, 10.0f    // near, far
		);

		// Set up uniform buffer objects (UBOs)
		// Camera UBO at binding point 0: view at index 0, projection at index 1
		UBO<glm::mat4, 2> matrices(0);
		matrices.updateData(view, 0);
		matrices.updateData(projection, 1);
		matrices.linkToProgram(terrain.getProgram(), "Matrices");

		// Light UBO at binding point 1: lightView at index 0, lightOrtho at index 1
		UBO<glm::mat4, 2> lightMatrices(1);
		lightMatrices.updateData(lightView, 0);
		lightMatrices.updateData(lightOrtho, 1);
		lightMatrices.linkToProgram(shadow.getProgram(), "LightMatrices");
		lightMatrices.linkToProgram(lighting.getProgram(), "LightMatrices");

		// Make FBOs
		Framebuffer gBuffer{ 3, RESX, RESY };
		if (!terrain.canDrawTo(gBuffer)) {
			throw std::runtime_error("Terrain shader outputs don't match the gBuffer");
		}
		Framebuffer sBuffer{ 1, 1024, 1024 };
		if (!shadow.canDrawTo(sBuffer)) {
			throw std::runtime_error("Shadow shader outputs don't match the sBuffer");
		}

		SDL_Surface* heightmapSurface = SDL_LoadBMP("heightmap.bmp");
		if (heightmapSurface == nullptr)
		{
			throw std::runtime_error(std::string("Failed to load heightmap: ") + SDL_GetError());
		}

		SDL_BlitSurface(heightmapSurface, NULL, SDL_GetWindowSurface(heightmapWindow), NULL);
		SDL_UpdateWindowSurface(heightmapWindow);

		// SDL event loop
		SDL_Event e;
		float mouseX = 0.0f, mouseY = 0.0f;
		float prevX = -1.0f, prevY = -1.0f;
		float brushRadius = 60.0f;

		bool quit = false;
		bool clicking = false;
		bool moved = false;
		bool leftMouseButton = true;


		while (!quit)
		{
			SDL_GetMouseState(&mouseX, &mouseY);
			moved = (mouseX != prevX || mouseY != prevY);

			if (SDL_GetMouseFocus() == heightmapWindow and clicking and moved) {
				SDL_Surface *winSurface = SDL_GetWindowSurface(heightmapWindow);
				drawCircle(glm::ivec2(mouseX, mouseY), brushRadius, winSurface, leftMouseButton);
				SDL_BlitSurface(winSurface, NULL, heightmapSurface, NULL);
				SDL_UpdateWindowSurface(heightmapWindow);
				plane.reuploadTexture(0, heightmapSurface);
			}
			prevX = mouseX;
			prevY = mouseY;

			//std::cout << "mouse coords: " << mouseX << ' ' << mouseY << '\n';
			while (SDL_PollEvent(&e))
			{
				switch (e.type)
				{
				case SDL_EVENT_WINDOW_CLOSE_REQUESTED: quit = true; break;
				case SDL_EVENT_MOUSE_BUTTON_DOWN:
				{
					if (e.button.button == SDL_BUTTON_LEFT) {
						// Left mouse button pressed
						leftMouseButton = true;
					}
					else if (e.button.button == SDL_BUTTON_RIGHT) {
						// Right mouse button pressed
						leftMouseButton = false;
					}
					clicking = true;
					break;
				}
				case SDL_EVENT_MOUSE_BUTTON_UP:
				{
					clicking = false;
					break;
				}
				case SDL_EVENT_KEY_DOWN:
				{
					if (e.key.key >= SDLK_0 && e.key.key <= SDLK_9) {
						int n = (int)(e.key.key - SDLK_0);
						if (n == 0) n = 10;
						brushRadius = n * 20.0f;
						std::cout << "Brush radius: " << brushRadius << '\n';
					}
					switch (e.key.key)
					{

					case SDLK_SPACE:
					{

						break;
					}
					case SDLK_T:
					{

						break;
					}

					}
					break;
				}
				}
			}

			// Geometry Pass
			gBuffer.bind();
			gBuffer.clear(0.5f, 0.5f, 0.5f);
			glEnable(GL_DEPTH_TEST);
			// Render meshes into GBuffer textures
			for (size_t i = 0; i < scene.size(); i++) {
				terrain.draw(*scene[i].mesh, scene[i].model);
			}

			// Shadow Pass
			sBuffer.bind();
			sBuffer.clear(1.0f, 1.0f, 1.0f);
			glEnable(GL_DEPTH_TEST);
			// Render meshes into sBuffer
			for (size_t i = 0; i < scene.size(); i++) {
				shadow.draw(*scene[i].mesh, scene[i].model);
			}

			// Lighting Pass
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, RESX, RESY);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glDisable(GL_DEPTH_TEST);

			// Upload lightDir to light.frag
			lighting.setVec3Array("lightDir", 1, glm::value_ptr(lightDir));

			// Fetch GBuffer texture handles from Framebuffer class
			std::vector<GLuint> textures = gBuffer.getTextures();
			textures.push_back(sBuffer.getTextures()[0]);

			// Draw screen quad
			lighting.drawScreen(screen, textures);

			// Swap buffers
			SDL_GL_SwapWindow(window);
		}
		SDL_DestroySurface(heightmapSurface);
	}

	//Clean up, usual FILO stack manner
	SDL_GL_DestroyContext(context);
	SDL_DestroyWindow(heightmapWindow);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

void drawCircle(const glm::ivec2& mouseCoords, float radius, SDL_Surface* surface, bool add) {
	int width = surface->w;
	int height = surface->h;

	float brushStrength = 2;
	float distance;
	float weight;
	float new_color;

	int radius_int = (int)std::ceil(radius);

	// Loop through pixels within bounding box of circle, 
	// using LERP to determine weight value from 0.0 to 1.0
	for (int x = mouseCoords.x - radius_int; x <= mouseCoords.x + radius_int; x++) {
		for (int y = mouseCoords.y - radius_int; y <= mouseCoords.y + radius_int; y++) {
			// Skip pixels outside the surface
			if (x < 0 || y < 0 || x >= width || y >= height) continue;

			// Read current RGB value at pixel (only need R since image is grayscale)
			std::uint8_t r;
			SDL_ReadSurfacePixel(surface, x, y, &r, NULL, NULL, NULL);

			// weight = 1 - (euclidean distance from center / radius)
			distance = std::sqrt((mouseCoords.x - x) * (mouseCoords.x - x)
							 + (mouseCoords.y - y) * (mouseCoords.y - y));
			weight = glm::clamp(1.0f - distance / radius, 0.0f, 1.0f);

			if (add) {
				new_color = glm::clamp(r + weight * brushStrength, 0.0f, 255.0f);
			}
			else {
				new_color = glm::clamp(r - weight * brushStrength, 0.0f, 255.0f);
			}

			drawPixel(surface, x, y, glm::ivec3(new_color, new_color, new_color));
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