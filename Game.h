//
// Game.h
//
#pragma once


#include <fstream>
#include "PrimitiveBatch.h"
#include "DeviceResources.h"
#include "StepTimer.h"
#include "Light.h"
#include "Input.h"
#include "Camera.h"
#include "RenderTexture.h"
#include "GeometryData.h"
#include "ShadowMap.h"
#include "D3DClass.h"


// A basic game implementation that creates a D3D11 device and
// provides a game loop.
class Game final : public DX::IDeviceNotify
{
public:

    Game() noexcept(false);
    ~Game();

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic game loop
    void Tick();

    void TakeInput();

    // IDeviceNotify
    virtual void OnDeviceLost() override;
    virtual void OnDeviceRestored() override;

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);
#ifdef DXTK_AUDIO
    void NewAudioDevice();
#endif

    // Properties
    void GetDefaultSize( int& width, int& height ) const;
	
private:

	struct MatrixBufferType
	{
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX projection;
	}; 

    // Init
    void SetupPrimitiveBatch();

    void Update(DX::StepTimer const& timer);
    bool RenderShadowMap(const XMMATRIX& lightViewMatrix, const XMMATRIX& lightProjectionMatrix);
    void Render();
    bool SetScreenBuffer(float red, float green, float blue, float alpha);
    bool GenerateScreenBuffer();
    //void Clear();
    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();
	void SetupGUI();
    void RegenerateTerrain();
    void ChangeWireframing();
    void ToggleWireframe();
    //void CastRay(const Ray& ray);
    //void CheckRaycast();

    // Device resources.
    //std::unique_ptr<DX::DeviceResources>    m_deviceResources;

    D3DClass* direct3D;
    // Texture renderTargets and buffers
    int currentScreenWidth, currentScreenHeight;
    ID3D11Texture2D* screenBuffer;
    ID3D11RenderTargetView* screenTargetView;
    ID3D11Texture2D* depthBuffer;
    ID3D11DepthStencilView* depthTargetView;
    RenderTextureClass* renderTexture;


    // Rendering loop timer.
    DX::StepTimer                           m_timer;

	//input manager. 
	Input									m_input;
	InputCommands							m_gameInputCommands;

    // DirectXTK objects.
    std::unique_ptr<DirectX::CommonStates>                                  m_states;
    std::unique_ptr<DirectX::BasicEffect>                                   m_batchEffect;	
    std::unique_ptr<DirectX::EffectFactory>                                 m_fxFactory;
    std::unique_ptr<DirectX::SpriteBatch>                                   m_sprites;
    std::unique_ptr<DirectX::SpriteFont>                                    m_font;

	// Scene Objects
	std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>  m_batch;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>                               m_batchInputLayout;
	std::unique_ptr<DirectX::GeometricPrimitive>                            m_testmodel;

    // Timer
    TimerClass* timer;

	//lights
	Light																	m_Light;

	//Cameras
	Camera																	m_Camera;

	//textures 
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_texture1;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_texture2;

	//Shaders
	//Shader																	m_BasicShaderPair;

    //Geometry
    bool wireframe = false;
    bool renderKDTree = false;
    bool rotateGeometry = false;
    int steps_initial = 10;
    int steps_refinement = 5;
    float depthfactor = 0.08f;
    float noiseScale = 10.f;

    int terrainCountX = 16;
    int terrainCountY = 16;
    int terrainCountZ = 16;

    float terrainMoveX = 0;
    float terrainMoveY = 0;
    float terrainMoveZ = 0;

    int terrainType;

    // KDTree
    PrimitiveBatch<VertexPositionColor>* primitiveBatch;
    BasicEffect* basicEffect;
    ID3D11InputLayout* inputLayout;
    
    //Scene.
    //Terrain																	m_Terrain;
    //ModelClass																m_BasicModel;
    //ModelClass																m_BasicModel2;
    //ModelClass																m_BasicModel3;

    // Shadow map terrain
    GeometryData* terrain = nullptr;
    GeometryData* plane = nullptr;
    KdTree tree;
    ShadowMap* shadowMap;


	//RenderTextures
	//RenderTexture*															m_FirstRenderPass;
	RECT																	m_fullscreenRect;
	RECT																	m_CameraViewRect;
	


#ifdef DXTK_AUDIO
    std::unique_ptr<DirectX::AudioEngine>                                   m_audEngine;
    std::unique_ptr<DirectX::WaveBank>                                      m_waveBank;
    std::unique_ptr<DirectX::SoundEffect>                                   m_soundEffect;
    std::unique_ptr<DirectX::SoundEffectInstance>                           m_effect1;
    std::unique_ptr<DirectX::SoundEffectInstance>                           m_effect2;
#endif
    

#ifdef DXTK_AUDIO
    uint32_t                                                                m_audioEvent;
    float                                                                   m_audioTimerAcc;

    bool                                                                    m_retryDefault;
#endif

    Matrix                                             m_world;
    Matrix                                             m_projection;
};