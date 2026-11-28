#include "mesh.hpp"
#include "shader.hpp"
#include "framebuffer.hpp"
#include "ubo.hpp"
#include "pbd.hpp"
#include "cubemap.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
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
	float reflectivity = 0.0f;
};

struct TrackPiece {
	Mesh* mesh;
	std::string file;
	glm::mat4 end;   // transform from this piece's start to its end (OpenGL coords)
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
	window = SDL_CreateWindow("Marble Race", RESX, RESY, SDL_WINDOW_OPENGL | SDL_WINDOW_INPUT_FOCUS);
	if (window == nullptr)
	{
		std::cerr << "Failed to create window: " << SDL_GetError();
		SDL_Quit();
		return 2;
	}
	context = SDL_GL_CreateContext(window);
	SDL_GL_SetSwapInterval(1);
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
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	glClearDepth(1.0f);
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);

	// This code in block so that mesh and shader destructors run before
	// GL_DestroyContext
	{
		// Make array of texture file paths
		std::vector<std::string> texturePaths1 = {
			"grassDiffuse.bmp",
			"grassNorm.bmp"
		};
		std::vector<std::string> texturePaths2 = {
			"trackDiffuse.bmp",
			"marbleNorm.bmp"
		};

		// Load meshes, textures, and shaders
		Mesh marbleRed("Sphere.txt", { "marbleRed.bmp", "marbleNorm.bmp" });
		Mesh marbleBlue("Sphere.txt", { "marbleBlue.bmp", "marbleNorm.bmp" });
		Mesh marbleGreen("Sphere.txt", { "marbleGreen.bmp", "marbleNorm.bmp" });
		Mesh marbleYellow("Sphere.txt", { "marbleYellow.bmp", "marbleNorm.bmp" });
		std::vector<Mesh*> marbleMeshes = { &marbleRed, &marbleBlue, &marbleGreen, &marbleYellow };

		Mesh floor("Floor.txt", texturePaths1);
		Mesh straightMesh("Straight.txt", texturePaths2);
		Mesh dropMesh("Drop.txt", texturePaths2);
		Mesh curveLMesh("CurveL.txt", texturePaths2);
		Mesh curveRMesh("CurveR.txt", texturePaths2);

		float floorAngle = 0.0f;
		std::vector<RenderObject> scene = {
			{ &floor, glm::mat4(1.0f) }
		};

		TrackPiece straight{ &straightMesh, "Straight.txt", glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, 0.0f))};
		TrackPiece drop{ &dropMesh, "Drop.txt", glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, -0.75f, 0.0f))};
		TrackPiece curveL{ &curveLMesh, "CurveL.txt",
			glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(1.5f, 0.0f, -1.5f)), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)) };
		TrackPiece curveR{ &curveRMesh, "CurveR.txt",
			glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(1.5f, 0.0f, 1.5f)), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)) };

		std::vector<TrackPiece> course = {
			drop, drop, straight, curveL,
			drop, straight, curveL,
			drop, straight, curveR,
			drop, curveR,
			drop, straight, drop, straight
		};

		PBDSolver solver;

		// Place pieces end to end, each starting where the previous one ended
		glm::mat4 cursor = glm::translate(glm::mat4(1.0f), glm::vec3(-7.0f, 4.0f, 5.0f));
		for (size_t i = 0; i < course.size(); i++) {
			scene.push_back({ course[i].mesh, cursor });
			solver.addTriangleMesh(course[i].file, cursor);
			cursor = cursor * course[i].end;
		}

		// Set up PBD
		Particle marble;
		marble.pos = marble.prevPos = glm::vec3(-3.5f, 1.5f, -0.2f);
		marble.velocity = glm::vec3(0.0f);
		marble.radius = 0.2f;
		marble.invMass = 1.0f;
		// TODO: Automate spawn system, probably use funnel at
		// beginning
		glm::vec3 spawns[] = {
			glm::vec3(-6.7f, 4.5f, 4.75f), glm::vec3(-6.7f, 4.5f, 5.25f),
			glm::vec3(-6.2f, 4.5f, 4.75f), glm::vec3(-6.2f, 4.5f, 5.25f),
			glm::vec3(-6.7f, 4.9f, 4.75f), glm::vec3(-6.7f, 4.9f, 5.25f),
			glm::vec3(-6.2f, 4.9f, 4.75f), glm::vec3(-6.2f, 4.9f, 5.25f),
			glm::vec3(-6.7f, 5.4f, 4.75f), glm::vec3(-6.7f, 5.4f, 5.25f),
			glm::vec3(-6.2f, 5.4f, 4.75f), glm::vec3(-6.2f, 5.4f, 5.25f),
			glm::vec3(-6.7f, 5.8f, 4.75f), glm::vec3(-6.7f, 5.8f, 5.25f),
			glm::vec3(-6.2f, 5.8f, 4.75f), glm::vec3(-6.2f, 5.8f, 5.25f)
		};
		for (int i = 0; i < 4; i++) {
			marble.pos = marble.prevPos = spawns[i];
			solver.particles.push_back(marble);
		}

		std::vector<Particle> initialParticles = solver.particles; // snapshot for reset

		Mesh screen("Screen.txt", {});

		Shader geometry("geo.vert", "geo.frag", {"albedo", "normal"}, 3);
		Shader shadow("shadow.vert", "shadow.frag", {});
		std::vector<std::string> texture_names = { "color", "gbuff_normal", "position", "shadowMap" };
		Shader lighting("light.vert", "light.frag", texture_names);
		Cubemap skybox({ "skybox/px.bmp", "skybox/nx.bmp", "skybox/py.bmp",
				 "skybox/ny.bmp", "skybox/pz.bmp", "skybox/nz.bmp" });
		glm::vec3 cameraPos = glm::vec3(3.95f, 10.3f, 10.9f);

		// Construct view and projection matrices
		glm::mat4 view = glm::lookAt(
			cameraPos,   // Position
			glm::vec3(-2.0f, 0.5f, -1.0f),    // Looking at this point
			glm::vec3(0.0f, 1.0f, 0.0f)     // Up vector
		);
		glm::mat4 projection = glm::perspective(
			glm::radians(45.0f),
			static_cast<float>(RESX) / static_cast<float>(RESY),
			0.1f,
			200.0f
		);

		// Construct light matrices based on skybox's sun
		glm::vec3 lightDir = glm::vec3(0.44f, -0.4f, -0.8f);
		glm::vec3 lightTarget = glm::vec3(-2.0f, 1.5f, -1.0f);
		glm::mat4 lightView = glm::lookAt(
			lightTarget - 20.0f * lightDir,
			lightTarget,
			glm::vec3(0.0f, 1.0f, 0.0f)
		);
		glm::mat4 lightOrtho = glm::ortho(
			-9.0f, 9.0f,    // left, right
			-7.5f, 7.0f,    // bottom, top
			9.5f, 31.0f     // near, far
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

		const float PHYSICS_DT = 1.0f / 60.0f;
		float accumulator = 0.0f;
		Uint64 lastTime = SDL_GetTicksNS();

		// SDL event loop
		SDL_Event e;
		float mouseX = 0.0f, mouseY = 0.0f;
		bool quit = false;
		while (!quit)
		{
			// PBD update
			Uint64 now = SDL_GetTicksNS();
			float frameTime = (now - lastTime) / 1e9f;
			lastTime = now;
			accumulator += std::min(frameTime, 0.25f);

			while (accumulator >= PHYSICS_DT) {
				// Floor tilt (hold A / D)
				const bool* keys = SDL_GetKeyboardState(NULL);
				const float tiltSpeed = 15.0f; // degrees per second
				if (keys[SDL_SCANCODE_A]) floorAngle += tiltSpeed * PHYSICS_DT;
				if (keys[SDL_SCANCODE_D]) floorAngle -= tiltSpeed * PHYSICS_DT;
				floorAngle = glm::clamp(floorAngle, -45.0f, 45.0f);

				glm::mat4 floorModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.0f, 0.0f));
				floorModel = glm::rotate(floorModel, glm::radians(floorAngle), glm::vec3(0.0f, 0.0f, 1.0f));
				scene[0].model = glm::scale(floorModel, glm::vec3(10.0f, 1.0f, 10.0f));

				solver.groundPoint = glm::vec3(floorModel * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
				solver.groundNormal = glm::normalize(glm::vec3(floorModel * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f)));

				solver.step(PHYSICS_DT);
				accumulator -= PHYSICS_DT;
			}

			// Add spheres to the scene for particles
			std::vector<RenderObject> frameScene = scene;
			for (size_t i = 0; i < solver.particles.size(); i++) {
				const Particle& p = solver.particles[i];
				glm::mat4 model = glm::translate(glm::mat4(1.0f), p.pos);
				model = model * glm::mat4_cast(p.orientation);
				model = glm::scale(model, glm::vec3(p.radius));
				frameScene.push_back({ marbleMeshes[i % marbleMeshes.size()], model, 1.0f });
			}

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
						// Reset particles
						solver.particles = initialParticles;
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
			for (size_t i = 0; i < frameScene.size(); i++) {
				geometry.setFloat("reflectivity", frameScene[i].reflectivity);
				geometry.draw(*frameScene[i].mesh, frameScene[i].model);
			}

			// Shadow Pass
			sBuffer.bind();
			sBuffer.clear(1.0f, 1.0f, 1.0f);
			glEnable(GL_DEPTH_TEST);
			// Render meshes into sBuffer
			for (size_t i = 0; i < frameScene.size(); i++) {
				shadow.draw(*frameScene[i].mesh, frameScene[i].model);
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

			// Send sky and camera to lighting pass
			lighting.setMat4("invViewProj", glm::inverse(projection* view));
			lighting.setVec3Array("cameraPos", 1, glm::value_ptr(cameraPos));
			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.getTexture());
			lighting.setInt("skybox", 4);

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
