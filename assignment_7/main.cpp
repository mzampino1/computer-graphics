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

constexpr int RESX = 960;
constexpr int RESY = 540;

struct RenderObject {
	Mesh* mesh;
	glm::mat4 model;
};

int main(int argc, char** argv)
{
	//Window is created dynamically, so we use a window pointer
	SDL_Window* window = nullptr;
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
	if (window == nullptr)
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
		// Make array of texture file paths
		std::vector<std::string> texturePaths = {
			"grassDiffuse.bmp",
			"grassNorm.bmp"
		};

		// Load meshes, textures, and shaders
		Mesh suzanne("Suzanne.txt", texturePaths);
		Mesh floor("Floor.txt", texturePaths);

		glm::mat4 floorModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.0f, 0.0f));
		floorModel = glm::scale(floorModel, glm::vec3(10.0f, 1.0f, 10.0f));
		std::vector<RenderObject> scene = {
			{ &suzanne, glm::mat4(1.0f) },
			{ &floor,   floorModel }
		};

		Mesh screen("Screen.txt", {});

		Shader geometry("geo.vert", "geo.frag", {"albedo", "normal"}, 3);
		Shader shadow("shadow.vert", "shadow.frag", {});
		std::vector<std::string> texture_names = { "color", "gbuff_normal", "position", "shadowMap" };
		Shader lighting("light.vert", "light.frag", texture_names);

		// Construct view and projection matrices
		glm::mat4 view = glm::lookAt(
			glm::vec3(3.0f, 3.0f, 4.0f),  // Position
			glm::vec3(0.0f, 0.0f, 0.0f),  // Looking at origin
			glm::vec3(0.0f, 1.0f, 0.0f)   // Up vector
		);
		glm::mat4 projection = glm::perspective(
			glm::radians(45.0f),
			static_cast<float>(RESX) / static_cast<float>(RESY),
			0.1f,
			200.0f
		);

		// Construct lightView and ortho matrices
		glm::vec3 lightDir = glm::vec3(1.0f, -1.0f, -1.0f);
		glm::mat4 lightView = glm::lookAt(
			glm::vec3(-5.0f, 5.0f, 5.0f),
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 1.0f, 0.0f)
		);
		glm::mat4 lightOrtho = glm::ortho(
			-3.0f, 3.0f,   // left, right
			-3.0f, 3.0f,   // bottom, top
			0.1f, 15.0f    // near, far
		);

		// Set up uniform buffer objects (UBOs)
		// Camera UBO at binding point 0: view at index 0, projection at index 1
		UBO<glm::mat4, 2> matrices(0);
		matrices.updateData(view, 0);
		matrices.updateData(projection, 1);
		matrices.linkToProgram(geometry.getProgram(), "Matrices");

		// Light UBO at binding point 1: lightView at index 0, lightOrtho at index 1
		UBO<glm::mat4, 2> lightMatrices(1);
		lightMatrices.updateData(lightView, 0);
		lightMatrices.updateData(lightOrtho, 1);
		lightMatrices.linkToProgram(shadow.getProgram(), "LightMatrices");
		lightMatrices.linkToProgram(lighting.getProgram(), "LightMatrices");

		// Make FBOs
		Framebuffer gBuffer{ 3, RESX, RESY };
		if (!geometry.canDrawTo(gBuffer)) {
			throw std::runtime_error("Geometry shader outputs don't match the gBuffer");
		}
		Framebuffer sBuffer{ 1, 1024, 1024 };
		if (!shadow.canDrawTo(sBuffer)) {
			throw std::runtime_error("Shadow shader outputs don't match the sBuffer");
		}

		// SDL event loop
		SDL_Event e;
		float mouseX = 0.0f, mouseY = 0.0f;
		bool quit = false;
		while (!quit)
		{
			SDL_GetMouseState(&mouseX, &mouseY);
			//std::cout << "mouse coords: " << mouseX << ' ' << mouseY << '\n';
			while (SDL_PollEvent(&e))
			{
				switch (e.type)
				{
				case SDL_EVENT_QUIT: quit = true; break;
				case SDL_EVENT_MOUSE_BUTTON_DOWN:
				{

					break;
				}
				case SDL_EVENT_MOUSE_BUTTON_UP:
				{

					break;
				}
				case SDL_EVENT_KEY_DOWN:
				{
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
				geometry.draw(*scene[i].mesh, scene[i].model);
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
	}

	//Clean up, usual FILO stack manner
	SDL_GL_DestroyContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
