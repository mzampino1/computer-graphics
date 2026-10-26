#include "mesh.hpp"
#include "shader.hpp"
#include "framebuffer.hpp"

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
		// Make array of mesh pointers to render
		std::vector<Mesh*> meshes;
		// Make array of texture file paths
		std::vector<std::string> texturePaths = {
			"grassDiffuse.bmp",
			"grassNorm.bmp"
		};

		// Load meshes, textures, and shaders
		Mesh suzanne("Suzanne.txt", texturePaths);
		meshes.push_back(&suzanne);
		Mesh screen("Screen.txt", {});
		meshes.push_back(&screen);
		Shader geometry("geo.vert", "geo.frag", {"albedo", "normal"}, 3);
		std::vector<std::string> texture_names = { "color", "gbuff_normal", "position" };
		Shader lighting("light.vert", "light.frag", texture_names);

		// Construct view, projection, model matrices
		glm::mat4 view = glm::lookAt(
			glm::vec3(1.0f, 0.5f, 3.0f),  // Position
			glm::vec3(0.0f, 0.0f, 0.0f),  // Looking at origin
			glm::vec3(0.0f, 1.0f, 0.0f)   // Up vector
		);
		glm::mat4 projection = glm::perspective(
			glm::radians(45.0f),
			static_cast<float>(RESX) / static_cast<float>(RESY),
			0.1f,
			200.0f
		);
		glm::mat4 model = glm::mat4(1.0f);

		// Set up uniform buffer object (UBO)
		GLuint ubo;
		glGenBuffers(1, &ubo);
		glBindBuffer(GL_UNIFORM_BUFFER, ubo);

		// Allocate memory
		glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), NULL, GL_STATIC_DRAW);

		// Bind buffer
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);

		// View matrix at offset 0
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(view));

		// Projection matrix at offset 64
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(projection));

		// Set shader uniform block to binding point 0
		GLuint blockIndex = glGetUniformBlockIndex(geometry.getProgram(), "Matrices");
		glUniformBlockBinding(geometry.getProgram(), blockIndex, 0);

		// Make FBO
		Framebuffer gBuffer{ 3, RESX, RESY };
		if (!geometry.canDrawTo(gBuffer)) {
			throw std::runtime_error("Geometry shader outputs don't match the gBuffer");
		}

		// Initialize 10 point lights
		std::vector<glm::vec3> lightPositions(10);
		std::vector<glm::vec3> lightColors(10);

		for (int i = 0; i < 10; ++i) {
			// Spread lights across the scene in a circle
			float angle = i * (2.0f * 3.14159f / 10.0f);
			lightPositions[i] = glm::vec3(sin(angle) * 2.0f, 0.5f, cos(angle) * 2.0f);

			// Give each light a unique color
			lightColors[i] = glm::vec3(
				0.5f + 0.5f * cos(angle),
				0.5f + 0.5f * cos(angle - 2.094f),
				0.5f + 0.5f * cos(angle + 2.094f)
			);
		}
		// Angle to track rotation of lights
		float globalLightAngle = 0.0f;

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
			// Check if A or D is being held down and rotate accordingly
			const bool* keys = SDL_GetKeyboardState(nullptr);
			if (keys[SDL_SCANCODE_A]) globalLightAngle -= 0.001f;
			if (keys[SDL_SCANCODE_D]) globalLightAngle += 0.001f;

			// Apply rotation to lights before drawing
			for (int i = 0; i < 10; i++) {
				float angle = i * (2.0f * 3.14159f / 10.0f) + globalLightAngle;
				lightPositions[i] = glm::vec3(sin(angle) * 2.0f, 0.5f, cos(angle) * 2.0f);
			}

			// Geometry Pass
			gBuffer.bind();
			gBuffer.clear(0.5f, 0.5f, 0.5f);
			glEnable(GL_DEPTH_TEST);
			// Render Suzanne into GBuffer textures
			geometry.draw(*(meshes[0]), model);

			// Lighting Pass
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glDisable(GL_DEPTH_TEST);

			// Upload 10 light positions and colors to light.frag
			lighting.setVec3Array("lightPosition", 10, glm::value_ptr(lightPositions[0]));
			lighting.setVec3Array("lightColor", 10, glm::value_ptr(lightColors[0]));

			// Fetch GBuffer texture handles from Framebuffer class
			std::vector<GLuint> gBufferTextures = gBuffer.getTextures();

			// Draw screen quad
			lighting.drawScreen(*(meshes[1]), gBufferTextures);

			// Swap buffers
			SDL_GL_SwapWindow(window);
		}
		// Clean up UBO
		glDeleteBuffers(1, &ubo);
	}

	//Clean up, usual FILO stack manner
	SDL_GL_DestroyContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
