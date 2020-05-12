//
// Game.cpp
//
#include "pch.h"
#include "Game.h"
using namespace DirectX;

extern void ExitGame();
using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace ImGui;
using Microsoft::WRL::ComPtr;

namespace GraphicsConfig
{
	const bool FULL_SCREEN = false;
	const bool VSYNC_ENABLED = true;
	const float SCREEN_DEPTH = 5000.0f;
	const float SCREEN_NEAR = 0.01f;
	const int SHADOWMAP_WIDTH = 2048;
	const int SHADOWMAP_HEIGHT = 2048;
}

Game::Game() noexcept(false)
{
    //m_deviceResources = std::make_unique<DX::DeviceResources>();
    //m_deviceResources->RegisterDeviceNotify(this);

	screenBuffer = nullptr;
	screenTargetView = nullptr;
	depthBuffer = nullptr;
	depthTargetView = nullptr;
}

Game::~Game()
{
#ifdef DXTK_AUDIO
    if (m_audEngine)
    {
        m_audEngine->Suspend();
    }
#endif
}

void Game::SetupPrimitiveBatch()
{
	primitiveBatch = new PrimitiveBatch<VertexPositionColor>(direct3D->GetDeviceContext());

	basicEffect = new BasicEffect(direct3D->GetDevice());
	basicEffect->SetVertexColorEnabled(true);

	void const* shaderByteCode;
	size_t byteCodeLength;

	basicEffect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

	direct3D->GetDevice()->CreateInputLayout(VertexPositionColor::InputElements,
		VertexPositionColor::InputElementCount,
		shaderByteCode, byteCodeLength,
		&inputLayout);
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
	currentScreenWidth = width;
	currentScreenHeight = height;

	bool result;

	//Set up Direct3D
	direct3D = new D3DClass();
	if (!direct3D)
	{
		MessageBox(window, L"Direct3D Class not defined", L"Error", MB_OK);
	}

	result = direct3D->Initialize(currentScreenWidth, currentScreenHeight, GraphicsConfig::VSYNC_ENABLED, window, GraphicsConfig::FULL_SCREEN, GraphicsConfig::SCREEN_DEPTH, GraphicsConfig::SCREEN_NEAR);
	if (!result)
	{
		MessageBox(window, L"Could not initialize Direct3D", L"Error", MB_OK);
	}

	m_input.Initialise(window);

    //m_deviceResources->SetWindow(window, width, height);

    //m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    //m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

	//setup imgui.  its up here cos we need the window handle too
	//pulled from imgui directx11 example
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(window);		//tie to our window
	ImGui_ImplDX11_Init(direct3D->GetDevice(), direct3D->GetDeviceContext());	//tie to directx

	m_fullscreenRect.left = 0;
	m_fullscreenRect.top = 0;
	m_fullscreenRect.right = 800;
	m_fullscreenRect.bottom = 600;

	m_CameraViewRect.left = 500;
	m_CameraViewRect.top = 0;
	m_CameraViewRect.right = 800;
	m_CameraViewRect.bottom = 240;

	//setup light
	m_Light.SetAmbientColour(0.5f, 0.5f, 0.5f, 1.0f);
	m_Light.SetDiffuseColour(1.0f, 1.0f, 1.0f, 1.0f);
	m_Light.SetPosition(2.0f, 12.0f, 1.0f);
	m_Light.SetDirection(-1.0f, -1.0f, 0.0f);
	m_Light.SetLookAt(0.0f, 0.0f, 0.0f);
	m_Light.GenerateProjectionsMatrix(GraphicsConfig::SCREEN_DEPTH, GraphicsConfig::SCREEN_NEAR);
	
	// Skydome initialize
	skydome = new Skydome;
	skydome->Initialize(direct3D->GetDevice());
	skydomeShader = new SkydomeShader;
	skydomeShader->Initialize(direct3D->GetDevice(), window);

	// Create timer for rotation
	timer = new TimerClass();
	timer->Initialize();

	m_Camera.Initialize(direct3D->GetDevice());
	RegenerateTerrain();

	shadowMap = new ShadowMap(direct3D->GetDevice(), direct3D->GetCurrentSampleCount(), direct3D->GetCurrentQualityLevel());

	//Create RenderToTexture
	renderTexture = new RenderTextureClass();
	if (!renderTexture)
	{
		MessageBox(window, L"Could not initialize the render to texture object.", L"Error", MB_OK);
	}

	result = renderTexture->Initialize(direct3D->GetDevice(), GraphicsConfig::SHADOWMAP_WIDTH, GraphicsConfig::SHADOWMAP_HEIGHT, GraphicsConfig::SCREEN_DEPTH, GraphicsConfig::SCREEN_NEAR, direct3D->GetCurrentSampleCount(), direct3D->GetCurrentQualityLevel());
	if (!result)
	{
		MessageBox(window, L"Could not initialize the render to texture object.", L"Error", MB_OK);
	}

	// Create First Back Buffer
	GenerateScreenBuffer();

	// SetupPrimiive For cube selector
	SetupPrimitiveBatch();


#ifdef DXTK_AUDIO
    // Create DirectXTK for Audio objects
    AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default;
#ifdef _DEBUG
    eflags = eflags | AudioEngine_Debug;
#endif

    m_audEngine = std::make_unique<AudioEngine>(eflags);

    m_audioEvent = 0;
    m_audioTimerAcc = 10.f;
    m_retryDefault = false;

    m_waveBank = std::make_unique<WaveBank>(m_audEngine.get(), L"adpcmdroid.xwb");

    m_soundEffect = std::make_unique<SoundEffect>(m_audEngine.get(), L"MusicMono_adpcm.wav");
    m_effect1 = m_soundEffect->CreateInstance();
    m_effect2 = m_waveBank->CreateInstance(10);

    m_effect1->Play(true);
    m_effect2->Play();
#endif
}

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick()
{
	//take in input
	m_input.Update();								//update the hardware
	m_gameInputCommands = m_input.getGameInput();	//retrieve the input for our game
	
	//Update all game objects
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

	//Render all game content. 
    Render();

#ifdef DXTK_AUDIO
    // Only update audio engine once per frame
    if (!m_audEngine->IsCriticalError() && m_audEngine->Update())
    {
        // Setup a retry in 1 second
        m_audioTimerAcc = 1.f;
        m_retryDefault = true;
    }
#endif
}

bool Game::CastCanMoveRay(const Ray& ray, float maxRange)
{
	float hitfloat = 0.0f;

	KdTree::RayHitStruct hit1;

	return tree.hit(&ray, hitfloat, maxRange, hit1);
}


bool Game::CastShootRay(const Ray& ray, float maxRange)
{
	float hitfloat = 0.0f;

	KdTree::RayHitStruct hit1;
	if (tree.hit(&ray, hitfloat, maxRange, hit1))
	{
		hasHit = true;
		lastHitDistance = hit1.hitDistance;
		Vector3 lastHitPoint = hit1.hitPoint;
	}
	else
	{
		hasHit = false;
		lastHitDistance = -1;
		printf("Hit nothing.\n\r");
	}

	return hasHit;
}

void Game::ChangeWireframing()
{
	D3D11_FILL_MODE fillmode = wireframe ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;

	ID3D11RasterizerState* rasterState;
	D3D11_RASTERIZER_DESC rasterStateDesc;

	direct3D->GetDeviceContext()->RSGetState(&rasterState);
	rasterState->GetDesc(&rasterStateDesc);
	rasterStateDesc.FillMode = fillmode;

	direct3D->GetDevice()->CreateRasterizerState(&rasterStateDesc, &rasterState);
	direct3D->GetDeviceContext()->RSSetState(rasterState);

	rasterState->Release();
	rasterState = nullptr;
}
void Game::ToggleWireframe() {
	wireframe = !wireframe;
	ChangeWireframing();
}

void Game::RegenerateTerrain()
{
	tree.PurgeTriangles();

	delete terrain;
	GeometryData::TerrainType::Enum terrainSelect = static_cast<GeometryData::TerrainType::Enum>(terrainType);
	terrain = new GeometryData(terrainCountX, terrainCountY, terrainCountZ, terrainSelect, direct3D->GetDevice(), direct3D->GetDeviceContext(), &tree, noiseScale);
	terrain->worldMatrix = XMMatrixIdentity() * XMMatrixScaling(5.0f, 5.0f, 5.0f);
	//terrain->DebugPrint();

	delete terrainMap;
	terrainMap = new GeometryData(64, 16, 64, GeometryData::TerrainType::HEIGHT_MAP, direct3D->GetDevice(), direct3D->GetDeviceContext(), &tree, noiseScale);
	terrainMap->worldMatrix = XMMatrixIdentity() * XMMatrixScaling(50.0f, 10.f, 50.0f) * XMMatrixTranslation(0.0f, -5.0f, 0.0f);

	//delete sphere;
	//sphere = new GeometryData(16, 16, 16, GeometryData::TerrainType::CUBE, direct3D->GetDevice(), direct3D->GetDeviceContext(), &tree);
	//sphere->worldMatrix = XMMatrixIdentity() * XMMatrixTranslation(3.0f, 3.0f, 3.0f);
}

void Game::CastCanMoveRays() {
	// Use a single ray and move it to different directions if input keys are pressed
	Ray ray;
	ray.position = m_Camera.GetPosition();
	XMMATRIX newdir;
	m_Camera.GetViewMatrix(newdir);

	if (m_gameInputCommands.forward) {
		ray.direction = Matrix(newdir).Transpose().Backward();
		blockForward = CastCanMoveRay(ray, 1);
	}

	if (m_gameInputCommands.back) {
		ray.direction = Matrix(newdir).Transpose().Forward();
		blockBackward = CastCanMoveRay(ray, 1);
	}

	if (m_gameInputCommands.left) {
		ray.direction = Matrix(newdir).Transpose().Left();
		blockLeft = CastCanMoveRay(ray, 1);
	}

	if (m_gameInputCommands.right) {
		ray.direction = Matrix(newdir).Transpose().Right();
		blockRight = CastCanMoveRay(ray, 1);
	}
}

void Game::TakeInput() {
	CastCanMoveRays();

	m_Camera.DoMovement(&m_gameInputCommands, blockForward, blockLeft, blockRight, blockBackward);

	if (m_gameInputCommands.shoot)
	{
		Ray ray;
		ray.position = m_Camera.GetPosition();
		XMMATRIX newdir;
		m_Camera.GetViewMatrix(newdir);
		ray.direction = Matrix(newdir).Transpose().Backward();

		CastShootRay(ray, 10000);
	}

	//if (m_gameInputCommands.randomGenerate)
	//{
	//	m_Terrain.GenerateRandomHeightMap();
	//}

	//if (m_gameInputCommands.addRanGenerate)
	//{
	//	m_Terrain.GenerateAdditiveOrMultiplyNoiseMap();
	//}

	//if (m_gameInputCommands.smoothen)
	//{
	//	m_Terrain.SmoothenHeightMap();
	//}
}

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{
	TakeInput();

	tree.UpdateKDTree();

	//m_Terrain.Update();		//terrain update.  doesnt do anything at the moment. 

	//m_world = Matrix::Identity;

	/*create our UI*/
	SetupGUI();

#ifdef DXTK_AUDIO
    m_audioTimerAcc -= (float)timer.GetElapsedSeconds();
    if (m_audioTimerAcc < 0)
    {
        if (m_retryDefault)
        {
            m_retryDefault = false;
            if (m_audEngine->Reset())
            {
                // Restart looping audio
                m_effect1->Play(true);
            }
        }
        else
        {
            m_audioTimerAcc = 4.f;

            m_waveBank->Play(m_audioEvent++);

            if (m_audioEvent >= 11)
                m_audioEvent = 0;
        }
    }
#endif

  
	if (m_input.Quit())
	{
		ExitGame();
	}
}
#pragma endregion

#pragma region Frame Render

bool Game::RenderShadowMap(const XMMATRIX& lightViewMatrix, const XMMATRIX& lightProjectionMatrix)
{
	shadowMap->Prepare(direct3D->GetDeviceContext());

	//Geometry Goes here, Shadow Map RenderPass
	//Loop for multiple Geometry
	if (terrain->isGeometryGenerated)
	{
		terrain->SetVertexBuffer(direct3D->GetDeviceContext());
		shadowMap->Render(direct3D->GetDeviceContext(), terrain->GetVertexCount(), terrain->worldMatrix, lightViewMatrix, lightProjectionMatrix);
	}

	if (terrainMap->isGeometryGenerated)
	{
		terrainMap->SetVertexBuffer(direct3D->GetDeviceContext());
		shadowMap->Render(direct3D->GetDeviceContext(), terrainMap->GetVertexCount(), terrainMap->worldMatrix, lightViewMatrix, lightProjectionMatrix);
	}

	direct3D->SetBackBufferRenderTarget();
	direct3D->ResetViewport();

	return true;
}

// Draws the scene.
void Game::Render()
{
	static float totaltime = 0.0f;
	timer->Frame();
	static float rotation = 0.0f;

	float deltaTime = timer->GetFrameTime();
	totaltime += deltaTime;

	if (totaltime > 1000)
	{
		//fps = 1000.0f / deltaTime;
		totaltime = 0.0f;
	}

    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

	XMMATRIX viewMatrix, projectionMatrix, translateMatrix;
	XMMATRIX lightViewMatrix, lightProjectionMatrix;

	//Generate view matrix based on camera
	//m_Camera.DoMovement(input);
	m_Camera.Render();

	//Get world, view and proj matrices
	//direct3D->GetWorldMatrix(worldMatrix);
	m_Camera.GetViewMatrix(viewMatrix);
	direct3D->GetProjectionMatrix(projectionMatrix);
	direct3D->GetWorldMatrix(translateMatrix);

	//Lighting
	m_Light.GenerateViewMatrix();
	m_Light.GetViewMatrix(lightViewMatrix);
	m_Light.GetProjectionMatrix(lightProjectionMatrix);

	//Generate Shadow Map
	RenderShadowMap(lightViewMatrix, lightProjectionMatrix);

	//clear Buffer at beginning
	//direct3D->BeginScene(0.2f, 0.5f, 0.5f, 0.0f);
	SetScreenBuffer(0.5f, 0.5f, 0.5f, 1.0f);

	// Render Skybox
	direct3D->TurnOffCulling();
	direct3D->TurnZBufferOff();
	m_world = SimpleMath::Matrix::Identity * Matrix::CreateScale(50.f) * SimpleMath::Matrix::CreateTranslation(m_Camera.GetPosition());
	skydome->Render(direct3D->GetDeviceContext());
	skydomeShader->Render(direct3D->GetDeviceContext(), skydome->GetIndexCount(), m_world, viewMatrix, sky_projection, skydome->GetApexColor(), skydome->GetCenterColor());
	direct3D->TurnOnCulling();
	direct3D->TurnZBufferOn();

	//Render Geometry	
	if (terrain)
	{
		if (rotateGeometry)
		{
			terrain->worldMatrix *= XMMatrixRotationY(deltaTime * 0.0001f);
		}
		terrain->worldMatrix *= XMMatrixTranslation(terrainMoveX * deltaTime * 0.0001f, terrainMoveY * deltaTime * 0.0001f, terrainMoveZ * deltaTime * 0.0001f);

		terrain->Render(direct3D->GetDeviceContext(), viewMatrix, projectionMatrix, m_Camera.GetPosition(), steps_initial, steps_refinement, depthfactor, m_Light, shadowMap->GetShaderResourceView());
	}

	//Render Geometry	
	if (terrainMap)
	{
		//if (rotateGeometry)
		//{
		//	terrainMap->worldMatrix *= XMMatrixRotationY(deltaTime * 0.0001f);
		//}
		terrainMap->Render(direct3D->GetDeviceContext(), viewMatrix, projectionMatrix, m_Camera.GetPosition(), steps_initial, steps_refinement, depthfactor, m_Light, shadowMap->GetShaderResourceView());
	}

	// Draw KDTree
	if (renderKDTree) {
		basicEffect->SetWorld(XMMatrixIdentity());
		basicEffect->SetView(viewMatrix);
		basicEffect->SetProjection(projectionMatrix);
		basicEffect->Apply(direct3D->GetDeviceContext());
		direct3D->GetDeviceContext()->IASetInputLayout(inputLayout);

		primitiveBatch->Begin();
		tree.Draw(primitiveBatch, Colors::LightGreen);
		primitiveBatch->End();
	}

	//render our GUI
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	

    // Show the new frame.
	basicEffect->SetWorld(XMMatrixIdentity());
	basicEffect->SetView(viewMatrix);
	basicEffect->SetProjection(projectionMatrix);
	basicEffect->Apply(direct3D->GetDeviceContext());
	direct3D->GetDeviceContext()->IASetInputLayout(inputLayout);

	//Render KD-Tree
	//primitiveBatch->Begin();
	//tree.Draw(primitiveBatch, Colors::LightGreen);
	//primitiveBatch->End();

	ID3D11Texture2D* backBuffer;
	direct3D->swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<LPVOID*>(&backBuffer));
	direct3D->GetDeviceContext()->ResolveSubresource(backBuffer, 0, screenBuffer, 0, DXGI_FORMAT_R8G8B8A8_UNORM);

	//Output Buffer
	direct3D->EndScene();

    //m_deviceResources->Present();
}

bool Game::GenerateScreenBuffer()
{
	D3D11_TEXTURE2D_DESC textureDesc;
	HRESULT result;
	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
	D3D11_TEXTURE2D_DESC depthBufferDesc;
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;

	if (screenBuffer)
	{
		screenBuffer->Release();
		screenBuffer = nullptr;
	}

	if (screenTargetView)
	{
		screenTargetView->Release();
		screenTargetView = nullptr;
	}

	if (depthBuffer)
	{
		depthBuffer->Release();
		depthBuffer = nullptr;
	}

	if (depthTargetView)
	{
		depthTargetView->Release();
		depthTargetView = nullptr;
	}

	ZeroMemory(&textureDesc, sizeof(textureDesc));

	//Setup Render target texture desc
	textureDesc.Width = currentScreenWidth;
	textureDesc.Height = currentScreenHeight;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;

	//Multisample
	textureDesc.SampleDesc.Count = direct3D->GetCurrentSampleCount();
	textureDesc.SampleDesc.Quality = direct3D->GetCurrentQualityLevel();

	// Create the render target texture.
	result = direct3D->GetDevice()->CreateTexture2D(&textureDesc, nullptr, &screenBuffer);
	if (FAILED(result))
	{
		return false;
	}

	renderTargetViewDesc.Format = textureDesc.Format;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
	renderTargetViewDesc.Texture2D.MipSlice = 0;

	// Create the render target view.
	result = direct3D->GetDevice()->CreateRenderTargetView(screenBuffer, &renderTargetViewDesc, &screenTargetView);
	if (FAILED(result))
	{
		return false;
	}

	// Initialize the description of the depth buffer.
	ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));

	// Set up the description of the depth buffer.
	depthBufferDesc.Width = currentScreenWidth;
	depthBufferDesc.Height = currentScreenHeight;
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.ArraySize = 1;
	depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthBufferDesc.CPUAccessFlags = 0;
	depthBufferDesc.MiscFlags = 0;

	//Multisample
	depthBufferDesc.SampleDesc.Count = direct3D->GetCurrentSampleCount();
	depthBufferDesc.SampleDesc.Quality = direct3D->GetCurrentQualityLevel();

	// Create the texture for the depth buffer using the filled out description.
	result = direct3D->GetDevice()->CreateTexture2D(&depthBufferDesc, nullptr, &depthBuffer);
	if (FAILED(result))
	{
		return false;
	}

	// Initailze the depth stencil view description.
	ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));

	// Set up the depth stencil view description.
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
	depthStencilViewDesc.Texture2D.MipSlice = 0;

	// Create the depth stencil view.
	result = direct3D->GetDevice()->CreateDepthStencilView(depthBuffer, &depthStencilViewDesc, &depthTargetView);
	if (FAILED(result))
	{
		return false;
	}

	return true;
}

bool Game::SetScreenBuffer(float red, float green, float blue, float alpha)
{
	//m_deviceResources->PIXBeginEvent(L"Clear");

	XMMATRIX lightViewMatrix, lightProjectionMatrix;

	direct3D->BeginScene(red, green, blue, alpha);

	//Set texture as render target
	renderTexture->SetRenderTarget(direct3D->GetDeviceContext());
	direct3D->GetDeviceContext()->OMSetRenderTargets(1, &screenTargetView, depthTargetView);
	direct3D->GetDeviceContext()->RSSetViewports(1, &direct3D->viewport);

	//Clear rendertexture
	float color[4];

	color[0] = red;
	color[1] = green;
	color[2] = blue;
	color[3] = alpha;

	direct3D->GetDeviceContext()->ClearRenderTargetView(screenTargetView, color);

	direct3D->GetDeviceContext()->ClearDepthStencilView(depthTargetView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	//m_deviceResources->PIXEndEvent();

	return true;
}

// Helper method to clear the back buffers.
//void Game::Clear()
//{
//
//    // Clear the views.
//    auto context = direct3D->GetDeviceContext();
//    auto renderTarget = m_deviceResources->GetRenderTargetView();
//    auto depthStencil = m_deviceResources->GetDepthStencilView();
//
//    context->ClearRenderTargetView(renderTarget, Colors::CornflowerBlue);
//    context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
//    context->OMSetRenderTargets(1, &renderTarget, depthStencil);
//
//    // Set the viewport.
//    auto viewport = m_deviceResources->GetScreenViewport();
//    context->RSSetViewports(1, &viewport);
//
//}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
}

void Game::OnDeactivated()
{
}

void Game::OnSuspending()
{
#ifdef DXTK_AUDIO
    m_audEngine->Suspend();
#endif
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();

#ifdef DXTK_AUDIO
    m_audEngine->Resume();
#endif
}

void Game::OnWindowMoved()
{
    //auto r = m_deviceResources->GetOutputSize();
    //m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Game::OnWindowSizeChanged(int width, int height)
{
    //if (!m_deviceResources->WindowSizeChanged(width, height))
        //return;

    CreateWindowSizeDependentResources();
}

#ifdef DXTK_AUDIO
void Game::NewAudioDevice()
{
    if (m_audEngine && !m_audEngine->IsAudioDevicePresent())
    {
        // Setup a retry in 1 second
        m_audioTimerAcc = 1.f;
        m_retryDefault = true;
    }
}
#endif

// Properties
void Game::GetDefaultSize(int& width, int& height) const
{
    width = 1024;
    height = 768;
}
#pragma endregion

#pragma region Direct3D Resources - Initialization
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    auto context = direct3D->GetDeviceContext();
    auto device = direct3D->GetDevice();

    m_states = std::make_unique<CommonStates>(device);
    m_fxFactory = std::make_unique<EffectFactory>(device);
    m_sprites = std::make_unique<SpriteBatch>(context);
    m_font = std::make_unique<SpriteFont>(device, L"SegoeUI_18.spritefont");
	m_batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(context);

	//setup our terrain
	//m_Terrain.Initialize(device, 1000, 1000);

	//setup our test model
	//m_BasicModel.InitializeSphere(device);
	//m_BasicModel2.InitializeModel(device,"drone.obj");
	//m_BasicModel3.InitializeBox(device, 10.0f, 0.1f, 10.0f);	//box includes dimensions

	//load and set up our Vertex and Pixel Shaders
	//m_BasicShaderPair.InitStandard(device, L"light_vs.cso", L"light_ps.cso");

	////load Textures
	//CreateDDSTextureFromFile(device, L"seafloor.dds",		nullptr,	m_texture1.ReleaseAndGetAddressOf());
	//CreateDDSTextureFromFile(device, L"GrassTex.dds", nullptr,	m_texture2.ReleaseAndGetAddressOf());

	////Initialise Render to texture
	//m_FirstRenderPass = new RenderTexture(device, 800, 600, 1, 2);	//for our rendering, We dont use the last two properties. but.  they cant be zero and they cant be the same. 

	float backBufferWidth = 800;
	float backBufferHeight = 600;
	sky_projection = Matrix::CreatePerspectiveFieldOfView(XMConvertToRadians(90.f),
		backBufferWidth / backBufferHeight, 0.01f, 1000.f);

}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    //auto size = m_deviceResources->GetOutputSize();
    float aspectRatio = (3.f / 4.f);
    float fovAngleY = 70.0f * XM_PI / 180.0f;

    // This is a simple example of change that can be made when the app is in
    // portrait or snapped view.
    if (aspectRatio < 1.0f)
    {
        fovAngleY *= 2.0f;
    }

    // This sample makes use of a right-handed coordinate system using row-major matrices.
    //m_projection = Matrix::CreatePerspectiveFieldOfView(
    //    fovAngleY,
    //    aspectRatio,
    //    0.01f,
    //    100.0f
    //);
}

void Game::SetupGUI()
{

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Terrain Object Control");
	ImGui::SliderFloat("Depth Factor", &depthfactor, 0.001f, 0.01f);
	ImGui::SliderInt("Terrain Size X", &terrainCountX, 10, 128);
	ImGui::SliderInt("Terrain Size Y", &terrainCountY, 10, 128);
	ImGui::SliderInt("Terrain Size Z", &terrainCountZ, 10, 128);
	ImGui::SliderFloat("Terrain Move X", &terrainMoveX, -5.0, 5.0);
	ImGui::SliderFloat("Terrain Move Y", &terrainMoveY, -5.0, 5.0);
	ImGui::SliderFloat("Terrain Move Z", &terrainMoveZ, -5.0, 5.0);
	ImGui::SliderFloat("NoiseScale", &noiseScale, 10.f, 100.0f);
	ImGui::SliderInt("TerrainType", &terrainType, 0, 7);
	if (ImGui::Button("Regenerate Terrain")) {
		RegenerateTerrain();
	}
	ImGui::Checkbox("Rotate Geometry?", &rotateGeometry);
	ImGui::End();

	ImGui::Begin("Hit Detection");
	ImGui::Text("Press Space to Shoot.");
	ImGui::Text(hasHit ? "Has Hit!!" : "No Hit");
	if (hasHit) {
		ImGui::Text("Last Hit Distance:  %f", &lastHitDistance);
		ImGui::Text("Last Hit Point:  %f %f %f", &lastHitPoint.x, &lastHitPoint.y, &lastHitPoint.z);
	}
	ImGui::End();

	ImGui::Begin("Movement Debug");
	ImGui::Text(blockForward ? "Forward Blocked!!!" : "Press W to go Forward");
	ImGui::Text(blockBackward ? "Back Blocked!!!" : "Press S to go Backward");
	ImGui::Text(blockLeft ? "Left Blocked!!!" : "Press A to strafe Left");
	ImGui::Text(blockRight ? "Right Blocked!!!" : "Press D to strafe Right");
	ImGui::End();


	ImGui::Begin("KD Tree/ Wireframe Parameters");
	ImGui::Checkbox("RenderKDTree?", &renderKDTree);
	if (ImGui::Button("ToggleWireframe")) {
		ToggleWireframe();
	}

	ImGui::SliderInt("Initial Steps", &steps_initial, 0, 100);
	ImGui::SliderInt("Refinement Steps", &steps_refinement, 0, 20);
	ImGui::End();
}


void Game::OnDeviceLost()
{
    m_states.reset();
    m_fxFactory.reset();
    m_sprites.reset();
    m_font.reset();
	m_batch.reset();
	m_testmodel.reset();
    m_batchInputLayout.Reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
}
#pragma endregion
