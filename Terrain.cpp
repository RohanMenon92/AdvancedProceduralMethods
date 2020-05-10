#include "pch.h"
#include "Terrain.h"

using namespace DirectX::SimpleMath;

Terrain::Terrain()
{
	m_terrainGeneratedToggle = false;
}


Terrain::~Terrain()
{
}

bool Terrain::Initialize(ID3D11Device* device, int terrainWidth, int terrainHeight)
{
	noiseGen = Noise();

	int index;
	float height = 0.0;
	bool result;

	// Save the dimensions of the terrain.
	m_terrainWidth = terrainWidth;
	m_terrainHeight = terrainHeight;

	m_frequency = m_terrainWidth / 20;
	m_amplitude = 3.0;
	m_wavelength = 1;

	m_noiseX = 2.0;
	m_noiseY = 2.0;
	m_noiseAmplitude = 3.0;
	m_noiseScale = 0.01;

	m_noiseAddMulToggle = false;

	m_layerCount = 2;
	m_layerHeight = 0.3;
	m_layerNoised = false;
	m_layerSteps = true;


	deviceRef = device;

	// Create the structure to hold the terrain data.
	m_heightMap = new HeightMapType[m_terrainWidth * m_terrainHeight];
	if (!m_heightMap)
	{
		return false;
	}

	// Initialise the data in the height map (flat).
	for (int j = 0; j<m_terrainHeight; j++)
	{
		for (int i = 0; i<m_terrainWidth; i++)
		{
			index = (m_terrainHeight * j) + i;

			m_heightMap[index].x = (float)i;
			m_heightMap[index].y = (float)height;
			m_heightMap[index].z = (float)j;

		}
	}

	//even though we are generating a flat terrain, we still need to normalise it. 
	// Calculate the normals for the terrain data.
	result = CalculateNormals();
	if (!result)
	{
		return false;
	}

	// Initialize the vertex and index buffer that hold the geometry for the terrain.
	result = InitializeBuffers();
	if (!result)
	{
		return false;
	}

	
	return true;
}

void Terrain::Render(ID3D11DeviceContext * deviceContext)
{
	// Put the vertex and index buffers on the graphics pipeline to prepare them for drawing.
	RenderBuffers(deviceContext);
	deviceContext->DrawIndexed(m_indexCount, 0, 0);

	return;
}

bool Terrain::CalculateNormals()
{
	int i, j, index1, index2, index3, index, count;
	float vertex1[3], vertex2[3], vertex3[3], vector1[3], vector2[3], sum[3], length;
	Vector3* normals;
	

	// Create a temporary array to hold the un-normalized normal vectors.
	normals = new Vector3[(m_terrainHeight - 1) * (m_terrainWidth - 1)];
	if (!normals)
	{
		return false;
	}

	// Go through all the faces in the mesh and calculate their normals.
	for (j = 0; j<(m_terrainHeight - 1); j++)
	{
		for (i = 0; i<(m_terrainWidth - 1); i++)
		{
			index1 = (j * m_terrainHeight) + i;
			index2 = (j * m_terrainHeight) + (i + 1);
			index3 = ((j + 1) * m_terrainHeight) + i;

			// Get three vertices from the face.
			vertex1[0] = m_heightMap[index1].x;
			vertex1[1] = m_heightMap[index1].y;
			vertex1[2] = m_heightMap[index1].z;

			vertex2[0] = m_heightMap[index2].x;
			vertex2[1] = m_heightMap[index2].y;
			vertex2[2] = m_heightMap[index2].z;

			vertex3[0] = m_heightMap[index3].x;
			vertex3[1] = m_heightMap[index3].y;
			vertex3[2] = m_heightMap[index3].z;

			// Calculate the two vectors for this face.
			vector1[0] = vertex1[0] - vertex3[0];
			vector1[1] = vertex1[1] - vertex3[1];
			vector1[2] = vertex1[2] - vertex3[2];
			vector2[0] = vertex3[0] - vertex2[0];
			vector2[1] = vertex3[1] - vertex2[1];
			vector2[2] = vertex3[2] - vertex2[2];

			index = (j * (m_terrainHeight - 1)) + i;

			// Calculate the cross product of those two vectors to get the un-normalized value for this face normal.
			normals[index].x = (vector1[1] * vector2[2]) - (vector1[2] * vector2[1]);
			normals[index].y = (vector1[2] * vector2[0]) - (vector1[0] * vector2[2]);
			normals[index].z = (vector1[0] * vector2[1]) - (vector1[1] * vector2[0]);
		}
	}

	// Now go through all the vertices and take an average of each face normal 	
	// that the vertex touches to get the averaged normal for that vertex.
	for (j = 0; j<m_terrainHeight; j++)
	{
		for (i = 0; i<m_terrainWidth; i++)
		{
			// Initialize the sum.
			sum[0] = 0.0f;
			sum[1] = 0.0f;
			sum[2] = 0.0f;

			// Initialize the count.
			count = 0;

			// Bottom left face.
			if (((i - 1) >= 0) && ((j - 1) >= 0))
			{
				index = ((j - 1) * (m_terrainHeight - 1)) + (i - 1);

				sum[0] += normals[index].x;
				sum[1] += normals[index].y;
				sum[2] += normals[index].z;
				count++;
			}

			// Bottom right face.
			if ((i < (m_terrainWidth - 1)) && ((j - 1) >= 0))
			{
				index = ((j - 1) * (m_terrainHeight - 1)) + i;

				sum[0] += normals[index].x;
				sum[1] += normals[index].y;
				sum[2] += normals[index].z;
				count++;
			}

			// Upper left face.
			if (((i - 1) >= 0) && (j < (m_terrainHeight - 1)))
			{
				index = (j * (m_terrainHeight - 1)) + (i - 1);

				sum[0] += normals[index].x;
				sum[1] += normals[index].y;
				sum[2] += normals[index].z;
				count++;
			}

			// Upper right face.
			if ((i < (m_terrainWidth - 1)) && (j < (m_terrainHeight - 1)))
			{
				index = (j * (m_terrainHeight - 1)) + i;

				sum[0] += normals[index].x;
				sum[1] += normals[index].y;
				sum[2] += normals[index].z;
				count++;
			}

			// Take the average of the faces touching this vertex.
			sum[0] = (sum[0] / (float)count);
			sum[1] = (sum[1] / (float)count);
			sum[2] = (sum[2] / (float)count);

			// Calculate the length of this normal.
			length = sqrt((sum[0] * sum[0]) + (sum[1] * sum[1]) + (sum[2] * sum[2]));

			// Get an index to the vertex location in the height map array.
			index = (j * m_terrainHeight) + i;

			// Normalize the final shared normal for this vertex and store it in the height map array.
			m_heightMap[index].nx = (sum[0] / length);
			m_heightMap[index].ny = (sum[1] / length);
			m_heightMap[index].nz = (sum[2] / length);
		}
	}

	// Release the temporary normals.
	delete[] normals;
	normals = 0;

	return true;
}

void Terrain::Shutdown()
{
	// Release the index buffer.
	if (m_indexBuffer)
	{
		m_indexBuffer->Release();
		m_indexBuffer = 0;
	}

	// Release the vertex buffer.
	if (m_vertexBuffer)
	{
		m_vertexBuffer->Release();
		m_vertexBuffer = 0;
	}

	return;
}

bool Terrain::InitializeBuffers()
{
	VertexType* vertices;
	unsigned long* indices;
	D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData, indexData;
	HRESULT result;
	int index, i, j;
	int index1, index2, index3, index4; //geometric indices. 

	// Calculate the number of vertices in the terrain mesh.
	m_vertexCount = (m_terrainWidth - 1) * (m_terrainHeight - 1) * 6;

	// Set the index count to the same as the vertex count.
	m_indexCount = m_vertexCount;

	// Create the vertex array.
	vertices = new VertexType[m_vertexCount];
	if (!vertices)
	{
		return false;
	}

	// Create the index array.
	indices = new unsigned long[m_indexCount];
	if (!indices)
	{
		return false;
	}

	// Initialize the index to the vertex buffer.
	index = 0;

	for (j = 0; j<(m_terrainHeight - 1); j++)
	{
		for (i = 0; i<(m_terrainWidth - 1); i++)
		{
			index1 = (m_terrainHeight * j) + i;          // Bottom left.
			index2 = (m_terrainHeight * j) + (i + 1);      // Bottom right.
			index3 = (m_terrainHeight * (j + 1)) + i;      // Upper left.
			index4 = (m_terrainHeight * (j + 1)) + (i + 1);  // Upper right.

															 // Upper left.

			// check what texture coordinates are being passed o the render shader
			// For defining texture scaling in shader
			float tilingValue = m_terrainHeight * 0.1;

			vertices[index].position = Vector3(m_heightMap[index3].x, m_heightMap[index3].y, m_heightMap[index3].z);
			vertices[index].normal = Vector3(m_heightMap[index3].nx, m_heightMap[index3].ny, m_heightMap[index3].nz);
			vertices[index].texture = Vector2(m_heightMap[index3].x/ tilingValue, m_heightMap[index3].z/ tilingValue);
			indices[index] = index;
			index++;

			// Upper right.
			vertices[index].position = Vector3(m_heightMap[index4].x, m_heightMap[index4].y, m_heightMap[index4].z);
			vertices[index].normal = Vector3(m_heightMap[index4].nx, m_heightMap[index4].ny, m_heightMap[index4].nz);
			vertices[index].texture = Vector2(m_heightMap[index4].x / tilingValue, m_heightMap[index4].z / tilingValue);
			indices[index] = index;
			index++;

			// Bottom left.
			vertices[index].position = Vector3(m_heightMap[index1].x, m_heightMap[index1].y, m_heightMap[index1].z);
			vertices[index].normal = Vector3(m_heightMap[index1].nx, m_heightMap[index1].ny, m_heightMap[index1].nz);
			vertices[index].texture = Vector2(m_heightMap[index1].x / tilingValue, m_heightMap[index1].z / tilingValue);
			indices[index] = index;
			index++;

			// Bottom left.
			vertices[index].position = Vector3(m_heightMap[index1].x, m_heightMap[index1].y, m_heightMap[index1].z);
			vertices[index].normal = Vector3(m_heightMap[index1].nx, m_heightMap[index1].ny, m_heightMap[index1].nz);
			vertices[index].texture = Vector2(m_heightMap[index1].x / tilingValue, m_heightMap[index1].z / tilingValue);
			indices[index] = index;
			index++;

			// Upper right.
			vertices[index].position = Vector3(m_heightMap[index4].x, m_heightMap[index4].y, m_heightMap[index4].z);
			vertices[index].normal = Vector3(m_heightMap[index4].nx, m_heightMap[index4].ny, m_heightMap[index4].nz);
			vertices[index].texture = Vector2(m_heightMap[index4].x / tilingValue, m_heightMap[index4].z / tilingValue);
			indices[index] = index;
			index++;

			// Bottom right.
			vertices[index].position = Vector3(m_heightMap[index2].x, m_heightMap[index2].y, m_heightMap[index2].z);
			vertices[index].normal = Vector3(m_heightMap[index2].nx, m_heightMap[index2].ny, m_heightMap[index2].nz);
			vertices[index].texture = Vector2(m_heightMap[index2].x / tilingValue, m_heightMap[index2].z / tilingValue);


			indices[index] = index;
			index++;
		}
	}

	// Set up the description of the static vertex buffer.
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(VertexType) * m_vertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the vertex data.
	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	// Now create the vertex buffer.
	result = deviceRef->CreateBuffer(&vertexBufferDesc, &vertexData, &m_vertexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	// Set up the description of the static index buffer.
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * m_indexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the index data.
	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	// Create the index buffer.
	result = deviceRef->CreateBuffer(&indexBufferDesc, &indexData, &m_indexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	// Release the arrays now that the vertex and index buffers have been created and loaded.
	delete[] vertices;
	vertices = 0;

	delete[] indices;
	indices = 0;

	return true;
}

void Terrain::RenderBuffers(ID3D11DeviceContext * deviceContext)
{
	unsigned int stride;
	unsigned int offset;

	// Set vertex buffer stride and offset.
	stride = sizeof(VertexType);
	offset = 0;

	// Set the vertex buffer to active in the input assembler so it can be rendered.
	deviceContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);

	// Set the index buffer to active in the input assembler so it can be rendered.
	deviceContext->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	return;
}

bool Terrain::GenerateWaveHeightMap()
{
	bool result;

	int index;
	float height = 0.0;

	m_frequency = (6.283 / m_terrainHeight) / m_wavelength; //we want a wavelength of 1 to be a single wave over the whole terrain.  A single wave is 2 pi which is about 6.283

															//loop through the terrain and set the hieghts how we want. This is where we generate the terrain
															//in this case I will run a sin-wave through the terrain in one axis.

	for (int j = 0; j<m_terrainHeight; j++)
	{
		for (int i = 0; i<m_terrainWidth; i++)
		{
			index = (m_terrainHeight * j) + i;

			m_heightMap[index].x = (float)i;
			m_heightMap[index].y = (float)(cos((float)i *(m_frequency))*m_amplitude) * (float)(cos((float)j *(m_frequency))*m_amplitude);
			m_heightMap[index].z = (float)j;
		}
	}

	result = CalculateNormals();
	if (!result)
	{
		return false;
	}

	result = InitializeBuffers();
	if (!result)
	{
		return false;
	}
}

// Create Random height map
bool Terrain::GenerateRandomHeightMap()
{
	bool result;

	int index;
	float height = 0.0;

	for (int j = 0; j<m_terrainHeight; j++)
	{
		for (int i = 0; i<m_terrainWidth; i++)
		{
			// Check for edge terrain
			if (i != 0 && i != (m_terrainWidth - 1) && j != 0 && j != m_terrainHeight - 1) {
				index = (m_terrainHeight * j) + i;

				m_heightMap[index].x = (float)i;
				//m_heightMap[index].y = (float)(m_frequency * (rand() % (int)(m_amplitude * 9)));


				m_heightMap[index].y = (float) (i * m_noiseScale * m_noiseX, j * m_noiseScale * m_noiseY) * m_noiseAmplitude;
				m_heightMap[index].z = (float)j;
			}
		}
	}

	result = CalculateNormals();
	if (!result)
	{
		return false;
	}

	result = InitializeBuffers();
	if (!result)
	{
		return false;
	}
}
bool Terrain::GenerateAdditiveOrMultiplyNoiseMap()
{
	bool result;

	int index;
	float height = 0.0;

	for (int j = 0; j<m_terrainHeight; j++)
	{
		for (int i = 0; i<m_terrainWidth; i++)
		{
			index = (m_terrainHeight * j) + i;

			m_heightMap[index].x = (float)i;
			// Check for edge terrain
			if (i != 0 && i != (m_terrainWidth - 1) && j != 0 && j != m_terrainHeight - 1) {
				float noiseValue = noiseGen.Noise2D(i * m_noiseScale * m_noiseX, j * m_noiseScale * m_noiseY) * m_noiseAmplitude;
				m_heightMap[index].y = m_noiseAddMulToggle ? m_heightMap[index].y + noiseValue : m_heightMap[index].y * noiseValue;
			}
			m_heightMap[index].z = (float)j;
		}
	}

	result = CalculateNormals();
	if (!result)
	{
		return false;
	}

	result = InitializeBuffers();
	if (!result)
	{
		return false;
	}
}

bool Terrain::GenerateLayeredNoiseMap() {
	bool result;

	int index;
	float height = 0.0;

	for (int j = 0; j < m_terrainHeight; j++)
	{
		for (int i = 0; i < m_terrainWidth; i++)
		{
			index = (m_terrainHeight * j) + i;

			float layerNoise = (noiseGen.Noise2D(i * m_noiseScale * m_noiseX, j * m_noiseScale * m_noiseY) + 1)/2;

			m_heightMap[index].x = (float)i;
			// Check for edge terrain
			if (i != 0 && i != (m_terrainWidth - 1) && j != 0 && j != m_terrainHeight - 1) {

				float layerHeight = m_layerHeight * (float)(m_layerSteps ? (int)(layerNoise * m_layerCount) : layerNoise * m_layerCount);
				if (m_layerNoised) {
					m_heightMap[index].y = m_noiseAddMulToggle ? layerNoise + layerHeight : layerNoise * layerHeight;
				}
				else {
					m_heightMap[index].y = layerHeight;
				}
			}
			m_heightMap[index].z = (float)j;
		}
	}

	result = CalculateNormals();
	if (!result)
	{
		return false;
	}

	result = InitializeBuffers();
	if (!result)
	{
		return false;
	}
}

bool Terrain::SmoothenHeightMap() {
	bool result;

	int index;
	float height = 0.0;

	//m_frequency = (6.283 / m_terrainHeight) / m_wavelength; //we want a wavelength of 1 to be a single wave over the whole terrain.  A single wave is 2 pi which is about 6.283

	//loop through the terrain and set the hieghts how we want. This is where we generate the terrain
	//in this case I will run a sin-wave through the terrain in one axis.

	for (int j = 0; j<m_terrainHeight; j++)
	{
		for (int i = 0; i<m_terrainWidth; i++)
		{
			index = (m_terrainHeight * j) + i;

			float surroundAverage = m_heightMap[index].y;
			if (i != 0 && i != (m_terrainWidth - 1) && j != 0 && j != m_terrainHeight - 1) {
				surroundAverage = (m_heightMap[index - 1].y + m_heightMap[index + 1].y +  // middle row
					m_heightMap[index - (m_terrainHeight)-1].y + m_heightMap[index - (m_terrainHeight)].y + m_heightMap[index - (m_terrainHeight)+1].y +  // top row
					m_heightMap[index + (m_terrainHeight)-1].y + m_heightMap[index + (m_terrainHeight)].y + m_heightMap[index + (m_terrainHeight)+1].y)  // bottom row
					/8; // Divide by total number of points
			}
			  
			m_heightMap[index].x = (float)i;
			m_heightMap[index].y = (float)surroundAverage;
			m_heightMap[index].z = (float)j;
		}
	}

	result = CalculateNormals();
	if (!result)
	{
		return false;
	}

	result = InitializeBuffers();
	if (!result)
	{
		return false;
	}
}

bool Terrain::Update()
{
	return true; 
}

float* Terrain::GetWavelength()
{
	return &m_wavelength;
}

float* Terrain::GetAmplitude()
{
	return &m_amplitude;
}

float* Terrain::GetNoiseX()
{
	return &m_noiseX;
}

float* Terrain::GetNoiseY()
{
	return &m_noiseY;
}

float* Terrain::GetNoiseScale()
{
	return &m_noiseScale;
}

float* Terrain::GetNoiseAmplitude()
{
	return &m_noiseAmplitude;
}

bool* Terrain::GetNoiseMulToggle() 
{
	return &m_noiseAddMulToggle;
}

int* Terrain::GetLayerCount()
{
	return &m_layerCount;
}

float* Terrain::GetLayerHeight()
{
	return &m_layerHeight;
}

bool* Terrain::GetLayerNoise()
{
	return &m_layerNoised;
}
bool* Terrain::GetLayerSteps()
{
	return &m_layerSteps;
}