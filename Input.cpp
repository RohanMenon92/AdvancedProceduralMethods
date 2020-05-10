#include "pch.h"
#include "Input.h"


Input::Input()
{
}

Input::~Input()
{
}

void Input::Initialise(HWND window)
{
	m_keyboard = std::make_unique<DirectX::Keyboard>();
	m_mouse = std::make_unique<DirectX::Mouse>();
	m_mouse->SetWindow(window);
	m_quitApp = false;

	m_GameInput.forward		= false;
	m_GameInput.back		= false;
	m_GameInput.right		= false;
	m_GameInput.left		= false;
	m_GameInput.rotRight	= false;
	m_GameInput.rotLeft		= false;
}

void Input::Update()
{
	auto kb = m_keyboard->GetState();	//updates the basic keyboard state
	m_KeyboardTracker.Update(kb);		//updates the more feature filled state. Press / release etc. 
	auto mouse = m_mouse->GetState();   //updates the basic mouse state
	m_MouseTracker.Update(mouse);		//updates the more advanced mouse state. 

	if (kb.Escape)// check has escape been pressed.  if so, quit out. 
	{
		m_quitApp = true;
	}

	//A key
	if (kb.A)	m_GameInput.left = true;
	else		m_GameInput.left = false;
	
	//D key
	if (kb.D)	m_GameInput.right = true;
	else		m_GameInput.right = false;

	//W key
	if (kb.W)	m_GameInput.forward	 = true;
	else		m_GameInput.forward = false;

	//S key
	if (kb.S)	m_GameInput.back = true;
	else		m_GameInput.back = false;

	//Left key
	if (kb.Left)	m_GameInput.rotLeft = true;
	else		m_GameInput.rotLeft = false;

	//Right key
	if (kb.Right)	m_GameInput.rotRight = true;
	else		m_GameInput.rotRight = false;

	//UP key
	if (kb.Up)	m_GameInput.rotUp = true;
	else		m_GameInput.rotUp = false;

	//DOWN key
	if (kb.Down)	m_GameInput.rotDown = true;
	else		m_GameInput.rotDown = false;

	//wave
	if (kb.Space) m_GameInput.waveGenerate = true;
	else		m_GameInput.waveGenerate = false;

	//random
	if (kb.LeftControl) m_GameInput.randomGenerate = true;
	else		m_GameInput.randomGenerate = false;

	//additiveRandom
	if (kb.LeftAlt) m_GameInput.addRanGenerate = true;
	else		m_GameInput.addRanGenerate = false;

	//Plus Smoothing
	if (kb.B) m_GameInput.smoothen = true;
	else		m_GameInput.smoothen = false;
}

bool Input::Quit()
{
	return m_quitApp;
}

InputCommands Input::getGameInput()
{
	return m_GameInput;
}
