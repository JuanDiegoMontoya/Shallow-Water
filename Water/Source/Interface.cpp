#include "stdafx.h"
#include "shader.h"
#include "camera.h"
#include "input.h"
#include "Pipeline.h"
#include "Engine.h"
#include "Renderer.h"
#include "Interface.h"
#include "PipeWater.h"

namespace Interface
{
	namespace
	{
		bool activeCursor = true;
	}


	void Init()
	{
		Engine::PushRenderCallback(DrawImGui, 1);
		Engine::PushRenderCallback(Update, 2);
	}


	void Update()
	{
		if (Input::Keyboard().pressed[GLFW_KEY_GRAVE_ACCENT])
			activeCursor = !activeCursor;
		glfwSetInputMode(Engine::GetWindow(), GLFW_CURSOR, activeCursor ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
	}


	void DrawImGui()
	{
		{
			ImGui::Begin("Shaders");
			if (ImGui::Button("Recompile Phong Shader"))
			{
				delete Shader::shaders["flatPhong"];
				Shader::shaders["flatPhong"] = new Shader("flatPhong.vs", "flatPhong.fs");
			}
			if (ImGui::Button("Recompile Height Shader"))
			{
				delete Shader::shaders["height"];
				Shader::shaders["height"] = new Shader("height.vs", "height.fs");
			}
			if (ImGui::Button("Recompile HeightWater Shader"))
			{
				//delete Shader::shaders["heightWater"];
				Shader::shaders["heightWater"] = new Shader("height.vs", "heightWater.fs");
			}

			ImGui::End();
		}

		{
			ImGui::Begin("Info");

			ImGui::Text("FPS: %.0f (%.1f ms)", 1.f / Engine::GetDT(), 1000. * Engine::GetDT());
			ImGui::NewLine();
			glm::vec3 pos = Renderer::GetPipeline()->GetCamera(0)->GetPos();
			if (ImGui::InputFloat3("Camera Position", &pos[0], 2))
				Renderer::GetPipeline()->GetCamera(0)->SetPos(pos);
			pos = Renderer::GetPipeline()->GetCamera(0)->front;
			ImGui::Text("Camera Direction: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
			glm::vec3 eu = Renderer::GetPipeline()->GetCamera(0)->GetEuler();
			ImGui::Text("Euler Angles: %.0f, %.0f, %.0f", eu.x, eu.y, eu.z);
			pos = pos * .5f + .5f;
			ImGui::SameLine();
			ImGui::ColorButton("visualization", ImVec4(pos.x, pos.y, pos.z, 1.f));

			ImGui::NewLine();
			ImGui::Text("Cursor: %s", !activeCursor ? "False" : "True");

			ImGui::End();
		}

		{
			ImGui::Begin("Simulation");

			// only allow user to choose same value for X and Y axes (otherwise crash will occur)
			static glm::ivec2 gridRes(1);
			static int tempGridRes(1);
			ImGui::SliderInt("Grid Resolution", &tempGridRes, 1, 5000);
			if (ImGui::Button("New Grid"))
			{
				delete Renderer::GetWaterSim();
				Renderer::GetWaterSim() = new PipeWater(tempGridRes, 1, tempGridRes);
				Renderer::GetWaterSim()->Init();
			}
			if (ImGui::Button("Test 1"))
			{
				bool result = Renderer::GetWaterSim()->Test1();
				std::cout << "Test 1: " << std::boolalpha << result << std::endl;
			}

			static double curSum = 0;
			static double prevSum = 0;
			ImGui::Text("Current  Water: %2.2f", curSum);
			ImGui::Text("Previous Water: %2.2f", prevSum);
			if (ImGui::Button("Measure Water Volume"))
			{
				prevSum = curSum;
				curSum = Renderer::GetWaterSim()->GetWaterSum(); // sim->sum
			}

			ImGui::Checkbox("Pause Simulation", &Engine::GetPauseRef());
			ImGui::SliderFloat("s/Update", &Renderer::GetUpdateFrequencyRef(), .01f, .3f, "%.2f");
			if (ImGui::Button("Re-init Sim"))
				Renderer::GetWaterSim()->Init();
			if (ImGui::Button("1x Update Simulation"))
				Renderer::GetWaterSim()->Update();
			if (ImGui::Button("20x Update Simulation"))
				for (int i = 0; i < 20; i++)
					Renderer::GetWaterSim()->Update();
			if (ImGui::Button("200x Update Simulation"))
				for (int i = 0; i < 200; i++)
					Renderer::GetWaterSim()->Update();
			if (ImGui::Button("2000x Update Simulation"))
				for (int i = 0; i < 2000; i++)
					Renderer::GetWaterSim()->Update();

			ImGui::End();
		}

		{
			ImGui::Begin("Rendering");

			ImGui::SliderFloat("Water Refract Idx", &Renderer::WaterRefract(), 0, 5);
			ImGui::SliderFloat("Air Refract Idx", &Renderer::AirRefract(), 0, 5);

			ImGui::End();
		}
	}


	bool IsCursorActive()
	{
		return activeCursor;
	}
}