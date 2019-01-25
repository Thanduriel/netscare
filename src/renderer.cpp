#include "renderer.hpp"
#include "utils.hpp"
#include <iostream>
#include <windows.h>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <vector>

ID3D11Device* Device::m_device = nullptr;
ID3D11DeviceContext* Device::m_context = nullptr;
IDXGISwapChain* Device::m_swapChain = nullptr;
ID3D11RenderTargetView* Device::m_backbuffer = nullptr;
const DirectX::CommonStates* Device::m_commonStates = nullptr;
const Effect* Device::m_effect = nullptr;
const Texture* Device::m_texture = nullptr;

HWND Device::m_windowHandle = 0;

DWORD_PTR* Device::m_swapChainVtable = nullptr;
D3D11PresentHook Device::m_orgPresent = nullptr;

PipeNode Device::m_pipeNode{};

using namespace std;

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

	return m_swapChainVtable;
}


HRESULT __stdcall Device::Present(IDXGISwapChain* This, UINT SyncInterval, UINT Flags)
{
	static size_t sizeBuffer = 0xBBBB;
	static ReadTask<size_t*> sizeTask(&sizeBuffer, 1);
	static std::vector<uint8_t> buffer(10);
	static bool isReadingData = false;
	static ReadTask<uint8_t*> dataTask(buffer.data(), 10);


	static bool init = true;
	if (init)
	{
		init = false;
		InitializeParent(This);
		m_pipeNode.addTask(sizeTask);
	}

	if (isReadingData)
	{
		if (dataTask.getState() != ReadTask<size_t*>::PENDING)
		{
			Utils::ResetInplace(sizeTask, &sizeBuffer, 1);
			m_pipeNode.addTask(sizeTask);
			isReadingData = false;
			delete m_texture;
			m_texture = new Texture(m_device, buffer.data(), sizeBuffer, -0.5f, -0.5f, 0.5f, 0.5f);
		}
	}
	else
	{
		if (sizeTask.getState() != ReadTask<char*>::PENDING)
		{
			buffer.resize(sizeBuffer);
			Utils::ResetInplace(dataTask, &buffer.front(), sizeBuffer);
			m_pipeNode.addTask(dataTask);
			isReadingData = true;
		}
	}

//	m_context->ClearRenderTargetView(m_backbuffer, color);
	Draw();
	
	return m_orgPresent(This, SyncInterval, Flags);
}

void Device::SetEffect(const Effect& _effect)
{
	m_effect = &_effect;

	m_context->IASetInputLayout(_effect.GetInputLayout());

	// Set the vertex and pixel shaders that will be used to render this triangle.
	m_context->VSSetShader(_effect.m_vertexShader, NULL, 0);
	m_context->PSSetShader(_effect.m_pixelShader, NULL, 0);

	m_context->PSSetSamplers(0, 1, &_effect.m_sampleState);
	float factor[4] = { 1.f,1.f,1.f,1.f };
	m_context->OMSetBlendState(_effect.m_blendState, factor, ~0); //D3D11_COLOR_WRITE_ENABLE_ALL & ~D3D11_COLOR_WRITE_ENABLE_ALPHA
}

DirectX::XMINT2 Device::GetBufferSize()
{
	RECT rect;
	if (!GetWindowRect(m_windowHandle, &rect))
		Utils::ShowError("GetWindowRect",GetLastError());

	return { rect.right - rect.left, rect.bottom - rect.top };
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
	m_context->Release();
	m_swapChain->Release();

	ID3D11Texture2D *pBackBuffer;
	_this->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	m_device->CreateRenderTargetView(pBackBuffer, NULL, &m_backbuffer);
	pBackBuffer->Release();

	m_commonStates = new DirectX::CommonStates(m_device);
	Effect* effect = new Effect(m_device,L"../shader/texture.vs",L"../shader/texture.ps");
	
	m_texture = new Texture(m_device,L"../texture/bolt.dds", -0.5f, -0.5f, 0.5f, 0.5f);
	SetEffect(*effect);
}