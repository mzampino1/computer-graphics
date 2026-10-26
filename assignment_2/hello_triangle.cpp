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

//  You will need to provide more attributes for me
//	Specifically, I want to have pretty colors :)
std::vector<float> mesh = {
	0.0f, 1.0f, 0.0f,
	-1.0f, -1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
	1.0f, 0.0f, 0.0f,  // Red
	0.0f, 1.0f, 0.0f,  // Green
	0.0f, 0.0f, 1.0f   // Blue
};

GLuint buildShaders(const char* vertShader, const char* fragShader)
{
	GLuint v = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(v, 1, &vertShader, NULL);

	glCompileShader(v);
	// check for shader compile errors
	int success;
	char infoLog[512];
	glGetShaderiv(v, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(v, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
		return 0;
	}

	GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(f, 1, &fragShader, NULL);

	glCompileShader(f);
	// check for shader compile errors
	glGetShaderiv(f, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(f, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
		return 0;
	}

	GLuint p = glCreateProgram();
	glAttachShader(p, v);
	glAttachShader(p, f);
	glLinkProgram(p);

	glGetProgramiv(p, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(p, 512, NULL, infoLog);
		std::cout << "linking failed: " << infoLog << std::endl;
		return 0;
	}

	glDeleteShader(v);
	glDeleteShader(f);

	return p;
}

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
	glClearColor(0.0f, 1.0f, 1.0f, 1.0f);

	//Need to setup a shader program and a mesh
	GLuint vbo, vao, program;
	GLsizei vertexCount = 3;

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * mesh.size(), &mesh[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)(3*3*sizeof(float)));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	
	//NOTE, this is an approach to checking for general OpenGL errors
	//	Depending on your system, sometimes there are starting errors to clear out
	//	But, you can verify by adding additional queries to print C-strings to a stream..
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR)
	{
		std::cout << "vao errs present: " << err << '\n';
	}

	//HERE -- Step 2 in Assignment, build two shaders (vertex and fragment)
	//	Load them into these strings, as is, errors will happen
	
	std::string vProg, fProg;

	// Load vertex shader
	std::string line;
	std::ifstream vert_file{ "vert.vert" };
	if (vert_file)
	{
		while (std::getline(vert_file, line))
		{
			vProg += line + "\n";
		}
	}

	// Load fragment shader
	std::ifstream frag_file{ "frag.frag" };
	if (frag_file)
	{
		while (std::getline(frag_file, line))
		{
			fProg += line + "\n";
		}
	}

	program = buildShaders(vProg.c_str(), fProg.c_str());

	if (!program)
	{
		std::cerr << "My compiler senses are tingling, something's going on here ~(*o*)~\n";
		return 3;
	}

	//SDL is handling many types of events in the background for us
	//	e.g., hardware input, window events, etc..
	//	Even if we are not using all of these events,
	//		we need to consume them from the queue they are added to
	//	Once we consume them, we can check them for anything concerning us
	//		For instance, if the window close button was clicked..
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
		//Clear the framebuffer before drawing into it
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBindVertexArray(vao);
		glUseProgram(program);
		glDrawArrays(GL_TRIANGLES, 0, vertexCount);

		//New call for swapping buffers
		SDL_GL_SwapWindow(window);
	}

	//Clean up, FILO
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);

	glDeleteProgram(program);

	//Clean up, usual FILO stack manner
	SDL_GL_DestroyContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
