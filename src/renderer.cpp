#include "renderer.hpp"
#include <iostream>
#include <windows.h>
#include <stdio.h>
#include <fstream>
#include <sstream>

ID3D11Device* Device::m_device = nullptr;
ID3D11DeviceContext* Device::m_context = nullptr;
IDXGISwapChain* Device::m_swapChain = nullptr;
ID3D11RenderTargetView* Device::m_backbuffer = nullptr;
const DirectX::CommonStates* Device::m_commonStates = nullptr;
const Effect* Device::m_effect = nullptr;
const Texture* Device::m_texture = nullptr;

DWORD_PTR* Device::m_swapChainVtable = nullptr;
D3D11PresentHook Device::m_orgPresent = nullptr;

PipeNode Device::m_pipeNode{};

using namespace std;

void showErrr(const char* caption, DWORD error) {
	LPSTR msg = 0;
	if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		error,
		0,
		(LPSTR)&msg, // muss anscheinend so, steht in der Doku
		0,
		NULL) == 0) {
		MessageBox(NULL, "Cant parse Error", caption, MB_OK | MB_ICONERROR);
		return;
	}
	MessageBox(NULL, msg, caption, MB_OK | MB_ICONERROR);
	if (msg) {
		LocalFree(msg);
		msg = 0;
	}
}

bool Device::Initialize()
{
	cout << "Creating Device & Swapchain" << endl;


	HWND hWnd = GetForegroundWindow();
	if (hWnd == nullptr)
		return false;


	cout << "" << endl;
	cout << "HWND:				0x" << hex << hWnd << endl;


	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = hWnd;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.Windowed = (GetWindowLong(hWnd, GWL_STYLE) & WS_POPUP) != 0 ? FALSE : TRUE;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	if (FAILED(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, &featureLevel, 1, D3D11_SDK_VERSION, &swapChainDesc, &m_swapChain, &m_device, NULL, &m_context)))
	{
		MessageBox(hWnd, "Unable to create pDevice & pSwapChain!", "FATAL ERROR", MB_ICONERROR);
		return false;
	}


	cout << "m_swapChain:			0x" << hex << m_swapChain << endl;
	cout << "m_device:			0x" << hex << m_device << endl;
	cout << "m_pContext:			0x" << hex << m_context << endl;

	m_swapChainVtable = (DWORD_PTR*)m_swapChain;
	m_swapChainVtable = (DWORD_PTR*)m_swapChainVtable[0];

	cout << "m_swapChainVtable:		0x" << hex << m_swapChainVtable << endl;

//	DWORD dwOld;
//	VirtualProtect(nullptr, 2, PAGE_EXECUTE_READWRITE, &dwOld);

	return (m_swapChainVtable);
}


HRESULT __stdcall Device::Present(IDXGISwapChain* This, UINT SyncInterval, UINT Flags)
{
	static unsigned char rgba[4] = {255, 0, 0, 255};
	static ReadTask<unsigned char*> rT(rgba, 4);
	static bool init = true;
	if (init)
	{
		init = false;
		InitializeParent(This);
	}

	if (rT.getState() != rT.PENDING) {
		m_pipeNode.addTask(rT);
	}

	float color[] = {
		static_cast<float>(rgba[0]) / 255.f,
		static_cast<float>(rgba[1]) / 255.f,
		static_cast<float>(rgba[2]) / 255.f,
		static_cast<float>(rgba[3]) / 255.f};
	m_context->ClearRenderTargetView(m_backbuffer, color);
	Draw();
	
	return m_orgPresent(This, SyncInterval, Flags);
}

void Device::SetEffect(const Effect& _effect)
{
	m_effect = &_effect;

	m_context->IASetInputLayout(_effect.GetInputLayout());

	// Set the vertex and pixel shaders that will be used to render this triangle.
	m_context->VSSetShader(_effect.GetVertexShader(), NULL, 0);
	m_context->PSSetShader(_effect.GetPixelShader(), NULL, 0);

	m_context->PSSetSamplers(0, 1, &_effect.m_sampleState);
	float factor[4] = { 1.f,1.f,1.f,1.f };
	m_context->OMSetBlendState(_effect.m_blendState, factor, ~0); //D3D11_COLOR_WRITE_ENABLE_ALL & ~D3D11_COLOR_WRITE_ENABLE_ALPHA
}

void Device::Draw()
{

	// Render the triangle.
	SetEffect(*m_effect);
	m_texture->Draw(m_context);
}

void Device::InitializeParent(IDXGISwapChain* _this)
{
	//GET DEVICE
	_this->GetDevice(__uuidof(ID3D11Device), (void**)&m_device);

	//CHECKING IF DEVICE IS VALID
	cout << "New m_pDevice:			0x" << hex << m_device << endl;
	if (!m_device) return;

	//REPLACING CONTEXT
	m_device->GetImmediateContext(&m_context);

	//CHECKING IF CONTEXT IS VALID
	cout << "New m_context:			0x" << hex << m_context << endl;
	if (!m_context) return;

	cout << "" << endl;

	m_device->Release();
	// cout << "m_pDevice released" << endl;

	m_context->Release();
	// cout << "m_pContext released" << endl;

	m_swapChain->Release();
	// cout << "m_pSwapChain released" << endl;

//	MessageBox(nullptr, "Test wupp wupp", "Caption", MB_OK);

	ID3D11Texture2D *pBackBuffer;
	_this->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	m_device->CreateRenderTargetView(pBackBuffer, NULL, &m_backbuffer);
	pBackBuffer->Release();

	m_commonStates = new DirectX::CommonStates(m_device);

	Effect* effect = new Effect(m_device,L"../shader/texture.vs",L"../shader/texture.ps");
	m_texture = new Texture(m_device,L"../texture/bolt.dds", -0.5f, -0.5f, 0.5f, 0.5f);
	SetEffect(*effect);
}