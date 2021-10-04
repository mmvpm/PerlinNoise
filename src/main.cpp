#ifdef WIN32
#include <SDL.h>
#undef main
#else
#include <SDL2/SDL.h>
#endif

#include <GL/glew.h>

#include <cmath>
#include <string_view>
#include <stdexcept>
#include <iostream>
#include <chrono>
#include <vector>
#include <map>

#include "include/Camera.hpp"
#include "include/Perlin2DPlot.hpp"

#include <minwindef.h>
#undef max
#undef min
#undef far
#undef near

#ifdef __cplusplus
extern "C" {
#endif

__declspec(dllexport) DWORD NvOptimusEnablement = 1;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;

#ifdef __cplusplus
}
#endif

std::string to_string(std::string_view str) {
	return { str.begin(), str.end() };
}

void sdl2_fail(std::string_view message) {
	throw std::runtime_error(to_string(message) + SDL_GetError());
}

void glew_fail(std::string_view message, GLenum error) {
	throw std::runtime_error(to_string(message) + reinterpret_cast<const char *>(glewGetErrorString(error)));
}

const char vertex_shader_source[] = R"(
    #version 330 core

    uniform mat4 view;
    uniform mat4 transform_xz;
    uniform mat4 transform_yz;

    layout (location = 0) in float x_position;
    layout (location = 1) in float y_position;
    layout (location = 2) in float z_position;
    layout (location = 3) in vec4 in_color;

    out vec4 color;

    void main() {
        vec4 position = vec4(x_position, y_position, z_position, 1.f);
        gl_Position = view * transform_yz * transform_xz * position;
        color = in_color;
    }
)";

const char fragment_shader_source[] = R"(
    #version 330 core

    uniform int isoline_count;

    in vec4 color;

    layout (location = 0) out vec4 out_color;

    float isoline_size = 0.005;
    vec4 isoline_color = vec4(0, 0, 0, 1);

    void main() {
        bool on_isoline = false;
        for (float i = 1; i < isoline_count; ++i) {
            float level = i / isoline_count;
            if (level < color[2] && color[2] < level + isoline_size) {
                on_isoline = true;
                isoline_color = vec4(level, level, level, 1);
                break;
            }
        }

        if (on_isoline)
            out_color = isoline_color;
        else
            out_color = color;
    }
)";

GLuint create_shader(GLenum type, const char * source) {
	GLuint result = glCreateShader(type);
	glShaderSource(result, 1, &source, nullptr);
	glCompileShader(result);
	GLint status;
	glGetShaderiv(result, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		GLint info_log_length;
		glGetShaderiv(result, GL_INFO_LOG_LENGTH, &info_log_length);
		std::string info_log(info_log_length, '\0');
		glGetShaderInfoLog(result, info_log.size(), nullptr, info_log.data());
		throw std::runtime_error("Shader compilation failed: " + info_log);
	}
	return result;
}

GLuint create_program(GLuint vertex_shader, GLuint fragment_shader) {
	GLuint result = glCreateProgram();
	glAttachShader(result, vertex_shader);
	glAttachShader(result, fragment_shader);
	glLinkProgram(result);

	GLint status;
	glGetProgramiv(result, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		GLint info_log_length;
		glGetProgramiv(result, GL_INFO_LOG_LENGTH, &info_log_length);
		std::string info_log(info_log_length, '\0');
		glGetProgramInfoLog(result, info_log.size(), nullptr, info_log.data());
		throw std::runtime_error("Program linkage failed: " + info_log);
	}

	return result;
}

int main() try {
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
		sdl2_fail("SDL_Init: ");

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	SDL_Window * window = SDL_CreateWindow(
        "Graphics course homework 1",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		800, 600,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED
    );

	if (!window)
		sdl2_fail("SDL_CreateWindow: ");

	int width, height;
	SDL_GetWindowSize(window, &width, &height);

	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	if (!gl_context)
		sdl2_fail("SDL_GL_CreateContext: ");

	if (auto result = glewInit(); result != GLEW_NO_ERROR)
		glew_fail("glewInit: ", result);

	if (!GLEW_VERSION_3_3)
		throw std::runtime_error("OpenGL 3.3 is not supported");

	glClearColor(0.2f, 0.2f, 0.2f, 0.f);

	auto vertex_shader = create_shader(GL_VERTEX_SHADER, vertex_shader_source);
	auto fragment_shader = create_shader(GL_FRAGMENT_SHADER, fragment_shader_source);
	auto program = create_program(vertex_shader, fragment_shader);

	GLuint view_location = glGetUniformLocation(program, "view");
	GLuint transform_xz_location = glGetUniformLocation(program, "transform_xz");
	GLuint transform_yz_location = glGetUniformLocation(program, "transform_yz");
    GLuint isoline_count_location = glGetUniformLocation(program, "isoline_count");

    float time = 0.f;
    int frames_per_second = 0;
    auto last_frame_start = std::chrono::high_resolution_clock::now();

    GLuint vbo_x;
    GLuint vbo_y;
    GLuint vbo_z;
    GLuint vbo_color;
    glGenBuffers(1, &vbo_x);
    glGenBuffers(1, &vbo_y);
    glGenBuffers(1, &vbo_z);
    glGenBuffers(1, &vbo_color);

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_x);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, 0, (void *) 0);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_y);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, (void *) 0);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_z);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 0, (void *) 0);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_color);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void *) 0);

    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    glEnable(GL_DEPTH_TEST);

    Camera camera = Camera();
    Perlin2DPlot plot = Perlin2DPlot();

    std::map<SDL_Keycode, bool> button_down;

    bool running = true;
	while (running) {
        // handling events
		for (SDL_Event event; SDL_PollEvent(&event);) switch (event.type) {
		case SDL_QUIT:
			running = false;
			break;
		case SDL_WINDOWEVENT: switch (event.window.event) {
			case SDL_WINDOWEVENT_RESIZED:
				width = event.window.data1;
				height = event.window.data2;
				glViewport(0, 0, width, height);
				break;
			}
			break;
		case SDL_KEYDOWN:
			button_down[event.key.keysym.sym] = true;
			break;
		case SDL_KEYUP:
			button_down[event.key.keysym.sym] = false;
			break;
		}

		if (!running)
			break;

        // time
		auto now = std::chrono::high_resolution_clock::now();
		float dt = std::chrono::duration_cast<std::chrono::duration<float>>(now - last_frame_start).count();
		last_frame_start = now;
		time += dt;
        frames_per_second += 1;
        if (time - dt < std::trunc(time)) {
            std::cout << "FPS: " << frames_per_second << std::endl;
            frames_per_second = 0;
        }

        // handling keyboard input
        camera.angle.x += dt * (float) button_down[SDLK_LEFT];
        camera.angle.x -= dt * (float) button_down[SDLK_RIGHT];
        camera.angle.y += dt * (float) button_down[SDLK_UP];
        camera.angle.y -= dt * (float) button_down[SDLK_DOWN];

        camera.shift.x += dt * (float) button_down[SDLK_d];
        camera.shift.x -= dt * (float) button_down[SDLK_a];
        camera.shift.y += dt * (float) button_down[SDLK_r];
        camera.shift.y -= dt * (float) button_down[SDLK_f];
        camera.shift.z += dt * (float) button_down[SDLK_s];
        camera.shift.z -= dt * (float) button_down[SDLK_w];

        if (button_down[SDLK_EQUALS]) plot.improve_grid();
        if (button_down[SDLK_MINUS])  plot.degrade_grid();
        if (button_down[SDLK_0]) plot.increase_isoline_count();
        if (button_down[SDLK_9]) plot.decrease_isoline_count();

        plot.dynamic_update(); // applying changes

        // 3D view parameters
        float aspect_ratio = (float) width / (float) height; // in `while` for dynamic window resizing
        float near = 0.1;
        float far = 10;
        float right = near; // * std::tan(pi / 4) == 1
        float top = right / aspect_ratio;

		float view[16] = {
			near / right, 0.f,        0.f,                          0.f,
			0.f,          near / top, 0.f,                          0.f,
			0.f,          0.f,        -(far + near) / (far - near), -2 * far * near / (far - near),
			0.f,          0.f,        -1.f,                         0.f,
		};

        // for rotating in XZ
        float ax = std::cos(camera.angle.x);
        float bx = std::sin(camera.angle.x);
        float transform_xz[16] = {
             ax, 0.f, -bx, 0.f,
            0.f, 1.f, 0.f, 0.f,
             bx, 0.f,  ax, 0.f,
            0.f, 0.f, 0.f, 1.f,
        };

        // for rotating in YZ (+ camera shift)
        float ay = std::cos(camera.angle.y);
        float by = std::sin(camera.angle.y);
        float transform_yz[16] = {
            1.f, 0.f, 0.f, -camera.shift.x,
            0.f,  ay, -by, -camera.shift.y,
            0.f,  by,  ay, -camera.shift.z,
            0.f, 0.f, 0.f, 1.f,
        };

        // clearing from previous frame
        glClear(GL_COLOR_BUFFER_BIT);
        glClear(GL_DEPTH_BUFFER_BIT);

		glUseProgram(program);

        if (plot.is_xz_changed_with_reset()) {
            glBindBuffer(GL_ARRAY_BUFFER, vbo_x);
            glBufferData(GL_ARRAY_BUFFER, plot.vertices_x.size() * sizeof(float), plot.vertices_x.data(), GL_STREAM_COPY);

            glBindBuffer(GL_ARRAY_BUFFER, vbo_z);
            glBufferData(GL_ARRAY_BUFFER, plot.vertices_z.size() * sizeof(float), plot.vertices_z.data(), GL_STREAM_COPY);

            glBufferData(GL_ELEMENT_ARRAY_BUFFER, plot.vertex_indices.size() * sizeof(uint32_t), plot.vertex_indices.data(), GL_STREAM_COPY);
        }

        glBindBuffer(GL_ARRAY_BUFFER, vbo_y);
        glBufferData(GL_ARRAY_BUFFER, plot.vertices_y.size() * sizeof(float), plot.vertices_y.data(), GL_STREAM_COPY);

        glBindBuffer(GL_ARRAY_BUFFER, vbo_color);
        glBufferData(GL_ARRAY_BUFFER, plot.vertices_color.size() * sizeof(float), plot.vertices_color.data(), GL_STREAM_COPY);

        glUniform1i(isoline_count_location, plot.isoline_count);

        glUniformMatrix4fv(view_location, 1, GL_TRUE, view);

        glUniformMatrix4fv(transform_xz_location, 1, GL_TRUE, transform_xz);
        glUniformMatrix4fv(transform_yz_location, 1, GL_TRUE, transform_yz);
        glDrawElements(GL_TRIANGLES, plot.vertex_indices.size(), GL_UNSIGNED_INT, (void *) 0);

		SDL_GL_SwapWindow(window);
	}

	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
}
catch (std::exception const & e) {
	std::cerr << e.what() << std::endl;
	return EXIT_FAILURE;
}
