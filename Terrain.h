#pragma once
#include "Noise.h"

using namespace DirectX::SimpleMath;

class Terrain
{
private:
	ID3D11Device* deviceRef;

	struct VertexType
	{
		Vector3 position;
		Vector2 texture;
		Vector3 normal;
	};
	struct HeightMapType
	{
		float x, y, z;
		float nx, ny, nz;
	};

public:
	Terrain();
	~Terrain();

	bool Initialize(ID3D11Device*, int terrainWidth, int terrainHeight);
	void Render(ID3D11DeviceContext*);
	bool GenerateWaveHeightMap();
	bool GenerateRandomHeightMap();
	bool GenerateAdditiveOrMultiplyNoiseMap();
	bool GenerateLayeredNoiseMap();
	bool SmoothenHeightMap();
	bool IsEdgeIndex(int index);
	bool Update();

	// Getters for imgui
	float* GetWavelength();
	float* GetAmplitude();
	float* GetNoiseX();
	float* GetNoiseY();
	float* GetNoiseScale();
	float* GetNoiseAmplitude();
	bool* GetNoiseMulToggle();
	int* GetLayerCount();
	float* GetLayerHeight();
	bool* GetLayerNoise();
	bool* GetLayerSteps();

private:
	bool CalculateNormals();
	void Shutdown();
	void ShutdownBuffers();
	bool InitializeBuffers();
	void RenderBuffers(ID3D11DeviceContext*);
	

private:
	bool m_terrainGeneratedToggle;
	int m_terrainWidth, m_terrainHeight;
	ID3D11Buffer * m_vertexBuffer, *m_indexBuffer;
	int m_vertexCount, m_indexCount;
	float m_frequency, m_amplitude, m_wavelength;

	bool m_noiseAddMulToggle;
	float m_noiseX, m_noiseY, m_noiseAmplitude, m_noiseScale;
	
	int m_layerCount;
	float m_layerHeight;
	bool m_layerNoised, m_layerSteps;
	
	HeightMapType* m_heightMap;

	//arrays for our generated objects Made by directX
	std::vector<VertexPositionNormalTexture> preFabVertices;
	std::vector<uint16_t> preFabIndices;
	Noise noiseGen;
};

