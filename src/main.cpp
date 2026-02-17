#include <cstdio>

#define TINYCSOCKET_IMPLEMENTATION

// Platform headers (must come before GL on Windows)
#ifdef _WIN32
#define NOMINMAX
#include <winsock2.h>
#include <windows.h>
#endif

// tinycsocket (must come before SDL - SDL redefines main to SDL_main)
#include "tinycsocket.h"

// SDL2
#include <SDL.h>

// OpenGL
#include <GL/gl.h>

// ImGui
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

// byte-stuffing
#include "cobs.h"
#include "PPP.h"

// dartt-protocol
#include "dartt.h"
#include "dartt_sync.h"
#include "checksum.h"

// App
#include "ui.h"
#include "dartt_init.h"
#include "plotting.h"

#include <Eigen/Dense>

#include <algorithm>
#include <string>
#include "dartt_mctl_params.h"
#include "motor.h"

#define NUM_MOTORS 2

// Helper: case-insensitive extension check
static bool ends_with_ci(const std::string& str, const std::string& suffix) 
{
	if (suffix.size() > str.size()) 
	{
		return false;
	}
	std::string tail = str.substr(str.size() - suffix.size());
	std::transform(tail.begin(), tail.end(), tail.begin(), ::tolower);
	return tail == suffix;
}


double thresh_dbl(double in, double hi, double lo)
{
	if(in > hi)
	{
		return hi;
	}
	else if(in < lo)
	{
		return lo;
	}
	return in;
}

int main(int argc, char* argv[])
{
	(void)argc;
	(void)argv;

	// Drag-and-drop state

	// Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) 
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		return -1;
	}

	// GL attributes
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	// Create window
	SDL_Window* window = SDL_CreateWindow(
		"DARTT Dashboard",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		1280, 720,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
	);
	
	if (!window) 
	{
		printf("Window creation failed: %s\n", SDL_GetError());
		return -1;
	}

	// Create GL context
	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	if (!gl_context) 
	{
		printf("GL context creation failed: %s\n", SDL_GetError());
		return -1;
	}
	SDL_GL_MakeCurrent(window, gl_context);
	SDL_GL_SetSwapInterval(1); // VSync

	// Initialize ImGui
	if (!init_imgui(window, gl_context)) 
	{
		printf("ImGui initialization failed\n");
		return -1;
	}

	Plotter plot;
	int width = 0;
	int height = 0;
	SDL_GetWindowSize(window, &width, &height);
	plot.init(width, height);
	
	if (tcs_lib_init() != TCS_SUCCESS)
	{
		printf("Failed to initialize tinycsocket\n");
	}
	else
	{
		printf("Initialize tinycsocket library success\n");
	}
		
	
	//allocate ds buffers
	Motor m[NUM_MOTORS] = {Motor(0x1), Motor(0x0)};
	snprintf(m[0].socket.ip, sizeof(m[0].socket.ip), "192.168.0.25");
	m[0].socket.port = 5400;
	udp_connect(&m[0].socket);
	snprintf(m[1].socket.ip, sizeof(m[1].socket.ip), "192.168.0.26");
	m[1].socket.port = 5400;
	udp_connect(&m[1].socket);


	dartt_buffer_t read_range = 
	{
		.buf = m[0].ds.ctl_base.buf,
		.size = m[0].ds.ctl_base.size,
		.len = sizeof(uint32_t)*4
	};

	// Main loop
	bool running = true;
	while (running)
	{
		// Poll events
		SDL_Event event;
		while (SDL_PollEvent(&event)) 
		{
			ImGui_ImplSDL2_ProcessEvent(&event);
			if (event.type == SDL_QUIT) 
			{
				running = false;
			}
			if (event.type == SDL_WINDOWEVENT &&
				event.window.event == SDL_WINDOWEVENT_CLOSE &&
				event.window.windowID == SDL_GetWindowID(window))
			{
				running = false;
			}
		}

		// Start ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		bool comms_good = true;
		for(int i = 0; i < NUM_MOTORS; i++)
		{
			read_range.buf = m[i].ds.ctl_base.buf;		//register on the ctl base we need to read into
			int rc = dartt_read_multi(&read_range, &m[i].ds);
			if(rc != DARTT_PROTOCOL_SUCCESS)
			{
				//send stop condition - even forces on both spools
				comms_good = false;
			}	
		}
		double t1 = 200.;
		double t2 = 200.;
		if(comms_good == false)
		{
		}
		else
		{
			double p1,p2;
			p1 = m[0].dp_periph.theta_rem_m * 180 / ((double)(1<<14) * 3.14159265);
			p2 = m[1].dp_periph.theta_rem_m * 180 / ((double)(1<<14) * 3.14159265);

			printf("%f, %d, %f, %d\r\n", p1, m[0].dp_periph.iq, p2, m[1].dp_periph.iq);
		}

		m[0].dp_ctl.command_word = (int32_t)t1;
		m[1].dp_ctl.command_word = (int32_t)t2;
		thresh_dbl(t1, 400., 200.);

		for(int i = 0; i < NUM_MOTORS; i++)
		{
			dartt_buffer_t write = {
				.buf = m[i].ds.ctl_base.buf,
				.size = sizeof(uint32_t),
				.len = sizeof(uint32_t)
			};
			int rc = dartt_write_multi(&write, &m[i].ds);
		}



		SDL_GetWindowSize(window, &plot.window_width, &plot.window_height);	//map out
		plot.sys_sec = (float)(((double)SDL_GetTicks64())/1000.);	//outside of class, load the time in sec as timebase for signals that use it as default

		//add new frame of data to each line, as determined by UI
		for(int i = 0; i < plot.lines.size(); i++)
		{
			plot.lines[i].enqueue_data(plot.window_width);
		}
		

		// Render
		ImGui::Render();
		int display_w, display_h;
		SDL_GetWindowSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		plot.render();	//must position here
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		SDL_GL_SwapWindow(window);
	}

	// Save UI settings back to config
	// save_dartt_config("config.json", config);

	// Cleanup
	shutdown_imgui();
	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
