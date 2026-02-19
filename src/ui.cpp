#include "ui.h"
#include "spooler_robot.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include <SDL.h>
#include <cstdio>
#include <vector>
#include <string>
#include "colors.h"
#include "dartt_init.h"


bool init_imgui(SDL_Window* window, SDL_GLContext gl_context) 
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    if (!ImGui_ImplSDL2_InitForOpenGL(window, gl_context)) 
	{
        fprintf(stderr, "Failed to init ImGui SDL2 backend\n");
        return false;
    }

    if (!ImGui_ImplOpenGL3_Init(nullptr))
	{
        fprintf(stderr, "Failed to init ImGui OpenGL3 backend\n");
        return false;
    }

#ifdef __ANDROID__
    io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
    ImGui::GetStyle().TouchExtraPadding = ImVec2(8.0f, 8.0f);
#endif

    return true;
}

void shutdown_imgui()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

void render_socket_ui(SpoolerRobot& robot)
{
    ImGui::Begin("Socket Config");
    for (int i = 0; i < (int)robot.motors.size(); i++)
    {
        Motor& m = robot.motors[i];
        ImGui::PushID(i);
        ImGui::Text("Motor %d", i);
        ImGui::SameLine();
        if (m.socket.connected)
            ImGui::TextColored(ImVec4(0,1,0,1), "[Connected]");
        else
            ImGui::TextColored(ImVec4(1,0.3f,0.3f,1), "[Disconnected]");

        if (ImGui::InputText("IP", m.socket.ip, sizeof(m.socket.ip),
                             ImGuiInputTextFlags_EnterReturnsTrue))
            udp_connect(&m.socket);

        int port = m.socket.port;
        if (ImGui::InputInt("Port", &port, 0, 0))
        {
            if (port > 0 && port <= 65535)
            { m.socket.port = (uint16_t)port; udp_connect(&m.socket); }
        }
        ImGui::Separator();
        ImGui::PopID();
    }
    ImGui::End();
}

void render_telemetry_ui(SpoolerRobot& robot)
{
    ImGui::Begin("Telemetry");
    if (ImGui::BeginTable("telem", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("Motor");
        ImGui::TableSetupColumn("Pos (deg)");
        ImGui::TableSetupColumn("Iq");
        ImGui::TableSetupColumn("dQ");
        for (int i = 0; i < (int)robot.motors.size(); i++)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("%d", i);
            ImGui::TableSetColumnIndex(1); ImGui::Text("%.3f", robot.p[i]);
            ImGui::TableSetColumnIndex(2); ImGui::Text("%.1f", (double)robot.iq[i]);
			ImGui::TableSetColumnIndex(3); ImGui::Text("%.3f", robot.dp[i]);
        }
        ImGui::EndTable();
    }
	ImGui::Text("k");
	ImGui::SameLine();
	ImGui::InputScalar("##pctl_gain", ImGuiDataType_Float, &robot.k);
	ImGui::Text("kd");
	ImGui::SameLine();
	ImGui::InputScalar("##derivative_gain", ImGuiDataType_Float, &robot.kd);

	ImGui::Text("tmax");
	ImGui::SameLine();
	ImGui::InputScalar("##tmax", ImGuiDataType_Float, &robot.tmax);

	ImGui::Text("targ");
	ImGui::SameLine();
	ImGui::InputScalar("##targ", ImGuiDataType_Float, &robot.targ);
	

	if(ImGui::Button("Rezero"))
	{
		// printf("Do rezero subroutine\n");
		for(int i = 0; i < 100; i++)
		{
			bool worked = robot.write_zero_offset();
			if(worked == true)
			{
				continue;
			}
			else
			{
				printf("Fail iteration %d\n", i);
			}
			SDL_Delay(10);
		}
	}
	if(ImGui::Button("Do Oscillate"))
	{
		robot.do_oscillation = !robot.do_oscillation;
	}
	ImGui::SameLine();
	if(robot.do_oscillation)
	{
		ImGui::Text("oscillating...");
	}
	
    ImGui::End();
}
