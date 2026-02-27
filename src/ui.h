#ifndef DARTT_UI_H
#define DARTT_UI_H

#include <SDL.h>
#include "plotting.h"

enum {FORCE_MODE, PCTL_TYPED, PCTL_CURSOR};

class SpoolerRobot;

// Initialize ImGui (call after SDL/OpenGL setup)
bool init_imgui(SDL_Window* window, SDL_GLContext gl_context);

// Shutdown ImGui
void shutdown_imgui();

void render_socket_ui(SpoolerRobot& robot);
void render_telemetry_ui(SpoolerRobot& robot, int & mode);

#endif // DARTT_UI_H
