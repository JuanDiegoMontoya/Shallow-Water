#pragma once

// responsible for making stuff appear on the screen
class Pipeline;
class PipeWater;
namespace Renderer
{
	void Init();
	void CompileShaders();

	// interaction
	void Update();
	void DrawAll();
	void Clear();

	void drawQuad();
	void initCubeMap();
	void drawCubeMap();

	// debug
	void drawAxisIndicators();

	Pipeline* GetPipeline();
	float& GetUpdateFrequencyRef();

	PipeWater*& GetWaterSim();

	float& WaterRefract();
	float& AirRefract();
}