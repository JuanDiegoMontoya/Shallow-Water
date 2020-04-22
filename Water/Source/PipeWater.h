#pragma once

//https://tutcris.tut.fi/portal/files/4312220/kellomaki_1354.pdf p74
struct Pipe
{
	float flow = 0; // Q += A*(g/dx)*dh*dt
};

struct PipeUpdateArgs
{
	float g = 9.8;
	float dx = 1; // length of pipe
	float dt = .125;
};

struct SplashArgs
{
	glm::vec2 pos = glm::vec2(40, 80);
	float A = 30; // amplitude
	float b = .1; // falloff rate
};

class PipeWater
{
public:
	PipeWater(int x, int y, int z);
	~PipeWater();
	void Init();
	void Update();
	void Render();
	double GetWaterSum();
	void SplashySplashy(SplashArgs arg);

	bool Test1();
private:
	std::vector<glm::vec3> vertices; // order doesn't change
	std::vector<glm::vec2> vertices2d; // order doesn't change
	std::vector<GLuint> indices; // immutable basically
	class IBO* pIbo = nullptr;
	class VBO* pVbo = nullptr;
	class VAO* pVao = nullptr;

	// use 
	void initDepthTex();
	GLuint HeightTex = -1;
	struct cudaGraphicsResource* imageResource;
	struct cudaArray* arr;

	Pipe* hPGrid = nullptr; // horizontal (x axis)
	Pipe* vPGrid = nullptr; // vertical (z axis)
	Pipe* temphPGrid = nullptr; // temp
	Pipe* tempvPGrid = nullptr; // temp

	PipeUpdateArgs args;
	SplashArgs splash;

	const int X, Y, Z;
	const int blockSize;
	const int numBlocks;

	const int PBlockSize;
	const int hPNumBlocks;
	const int vPNumBlocks;

};