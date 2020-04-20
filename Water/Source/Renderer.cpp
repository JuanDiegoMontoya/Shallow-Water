#include "stdafx.h"
#include "vbo.h"
#include "vao.h"
#include "ibo.h"
#include "shader.h"
#include "camera.h"
#include "Pipeline.h"
#include "input.h"
#include "Engine.h"
#include "Vertices.h"
#include "Interface.h"
#include <stb_image.h>

#include "PipeWater.h"
#include "Renderer.h"

// initializes the gBuffer and its attached textures
namespace Renderer
{
	// private variables
	namespace
	{
		// post processing
		bool ppSharpenFilter = false;
		bool ppBlurFilter = false;
		bool ppEdgeDetection = false;
		bool ppChromaticAberration = false;

		// CA
		bool pauseSimulation = false;
		float updateFrequency = .01f; // seconds
		float timeCount = 0;

		PipeWater waterSim(2000, 1, 2000);
		int automataIndex = 5;

		// cubemap
		GLuint cubeTex;
		VAO* cubeVao = nullptr;
		VBO* cubeVbo = nullptr;

		Pipeline pipeline;
	}
	

	static void GLAPIENTRY
		GLerrorCB(GLenum source,
			GLenum type,
			GLuint id,
			GLenum severity,
			GLsizei length,
			const GLchar* message,
			const void* userParam)
	{
		//return; // UNCOMMENT WHEN DEBUGGING GRAPHICS

		// ignore non-significant error/warning codes
		if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

		std::cout << "---------------" << std::endl;
		std::cout << "Debug message (" << id << "): " << message << std::endl;

		switch (source)
		{
		case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
		case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
		case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
		} std::cout << std::endl;

		switch (type)
		{
		case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
		case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
		case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
		case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
		case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
		case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
		case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
		} std::cout << std::endl;

		switch (severity)
		{
		case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
		case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
		case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
		case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
		} std::cout << std::endl;
		std::cout << std::endl;
	}


	void Renderer::Init()
	{
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEPTH_TEST);

		// enable debugging stuff
		glDebugMessageCallback(GLerrorCB, NULL);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glFrontFace(GL_CCW);

		CompileShaders();

		pipeline.AddCamera(new Camera(kControlCam));
		pipeline.ClearColor = { 0, .2, .4 };

		initCubeMap();
		waterSim.Init();

		Engine::PushRenderCallback(DrawAll, 0);
		Engine::PushUpdateCallback(Update, 0);
	}


	void CompileShaders()
	{
		// initialize all of the shaders that will be used
		Shader::shaders["line"] = new Shader("line.vs", "line.fs");
		Shader::shaders["phong"] = new Shader("phong.vs", "phong.fs");
		Shader::shaders["postprocess"] = new Shader("postprocess.vs", "postprocess.fs");
		Shader::shaders["axis"] = new Shader("axis.vs", "axis.fs");
		Shader::shaders["light"] = new Shader("light.vs", "light.fs");
		Shader::shaders["skybox"] = new Shader("skybox.vs", "skybox.fs");
		Shader::shaders["flat"] = new Shader("flat.vs", "flat.fs");
		Shader::shaders["flatPhong"] = new Shader("flatPhong.vs", "flatPhong.fs");
		Shader::shaders["height"] = new Shader("height.vs", "height.fs");
		Shader::shaders["heightWater"] = new Shader("height.vs", "heightWater.fs");
	}


	void RecompileShaders()
	{
		for (auto& shader : Shader::shaders)
		{
			//if (shader.first == "postprocess")
			//	continue;
			std::string vs = shader.second->vsPath.c_str();
			std::string fs = shader.second->fsPath.c_str();
			delete shader.second;
			shader.second = new Shader(vs.c_str(), fs.c_str());
		}
	}


	void Update()
	{
		if (!pauseSimulation)
		{
			timeCount += Engine::GetDT();

			if (timeCount > updateFrequency)
			{
				waterSim.Update();
				timeCount = 0;
			}
		}
	}


	void Renderer::DrawAll()
	{
		PERF_BENCHMARK_START;
		glEnable(GL_FRAMEBUFFER_SRGB); // gamma correction

		if (Input::Keyboard().down[GLFW_KEY_LEFT_SHIFT])
		{
			if (Input::Keyboard().pressed[GLFW_KEY_1])
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			if (Input::Keyboard().pressed[GLFW_KEY_2])
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			if (Input::Keyboard().pressed[GLFW_KEY_3])
				glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
		}

		// update each camera
		if (!Interface::IsCursorActive())
			pipeline.GetCamera(0)->Update(Engine::GetDT());

		Clear();

		drawCubeMap();

		waterSim.Render();

		drawAxisIndicators();

		glDisable(GL_FRAMEBUFFER_SRGB);
		PERF_BENCHMARK_END;
	}


	void Renderer::Clear()
	{
		auto clr = pipeline.ClearColor;
		glClearColor(clr.r, clr.g, clr.b, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}


	void Renderer::drawAxisIndicators()
	{
		static VAO* axisVAO;
		static VBO* axisVBO;
		if (axisVAO == nullptr)
		{
			float indicatorVertices[] =
			{
				// positions			// colors
				0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // x-axis
				1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, // y-axis
				0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, // z-axis
				0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f
			};

			axisVAO = new VAO();
			axisVBO = new VBO(indicatorVertices, sizeof(indicatorVertices), GL_STATIC_DRAW);
			VBOlayout layout;
			layout.Push<float>(3);
			layout.Push<float>(3);
			axisVAO->AddBuffer(*axisVBO, layout);
		}
		/* Renders the axis indicator (a screen-space object) as though it were
			one that exists in the world for simplicity. */
		ShaderPtr currShader = Shader::shaders["axis"];
		currShader->Use();
		Camera* cam = pipeline.GetCamera(0);
		currShader->setMat4("u_model", glm::translate(glm::mat4(1), cam->GetPos() + cam->front * 10.f)); // add scaling factor (larger # = smaller visual)
		currShader->setMat4("u_view", cam->GetView());
		currShader->setMat4("u_proj", cam->GetProj());
		glClear(GL_DEPTH_BUFFER_BIT); // allows indicator to always be rendered
		axisVAO->Bind();
		glLineWidth(2.f);
		glDrawArrays(GL_LINES, 0, 6);
		axisVAO->Unbind();
	}


	Pipeline* GetPipeline()
	{
		return &pipeline;
	}


	int GetAutomatonIndex()
	{
		return automataIndex;
	}


	float& GetUpdateFrequencyRef()
	{
		return updateFrequency;
	}

	PipeWater* GetWaterSim()
	{
		return &waterSim;
	}


	// draws a single quad over the entire viewport
	void Renderer::drawQuad()
	{
		static unsigned int quadVAO = 0;
		static unsigned int quadVBO;
		if (quadVAO == 0)
		{
			float quadVertices[] =
			{
				// positions        // texture Coords
				-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
				-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
				 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
				 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
			};
			// setup plane VAO
			glGenVertexArrays(1, &quadVAO);
			glGenBuffers(1, &quadVBO);
			glBindVertexArray(quadVAO);
			glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
		}
		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);
	}


	void Renderer::initCubeMap()
	{
		glGenTextures(1, &cubeTex);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTex);

		cubeVao = new VAO();
		cubeVbo = new VBO(Vertices::skybox, sizeof(Vertices::skybox));
		VBOlayout layout;
		layout.Push<float>(3);
		cubeVao->AddBuffer(*cubeVbo, layout);

		std::vector<std::string> faces =
		{
			"./resources/Textures/skybox/right.jpg",
			"./resources/Textures/skybox/left.jpg",
			"./resources/Textures/skybox/top.jpg",
			"./resources/Textures/skybox/bottom.jpg",
			"./resources/Textures/skybox/front.jpg",
			"./resources/Textures/skybox/back.jpg"
		};
		int width, height, nrChannels;
		for (unsigned int i = 0; i < faces.size(); i++)
		{
			unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
			if (data)
			{
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
					0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
				);
				stbi_image_free(data);
			}
			else
			{
				std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
				stbi_image_free(data);
			}
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	}


	void drawCubeMap()
	{
		glDepthMask(GL_FALSE);
		ShaderPtr sr = Shader::shaders["skybox"];
		sr->Use();
		sr->setInt("skybox", 1);
		sr->setMat4("proj", pipeline.GetCamera(0)->GetProj());
		glm::mat4 view = pipeline.GetCamera(0)->GetView();
		view = glm::mat4(glm::mat3(view));
		sr->setMat4("view", view);
		cubeVao->Bind();
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTex);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glDepthMask(GL_TRUE);
	}
}