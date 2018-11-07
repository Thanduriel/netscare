#include "renderer.hpp"
#include <iostream>
#include <windows.h>
#include <stdio.h>
#include <fstream>

ID3D11Device* Device::m_device = nullptr;
ID3D11DeviceContext* Device::m_context = nullptr;
IDXGISwapChain* Device::m_swapChain = nullptr;
ID3D11RenderTargetView* Device::backbuffer = nullptr;

DWORD_PTR* Device::m_swapChainVtable = nullptr;
D3D11PresentHook Device::m_orgPresent = nullptr;

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

//	DWORD dwOld;
//	VirtualProtect(nullptr, 2, PAGE_EXECUTE_READWRITE, &dwOld);

	return (m_swapChainVtable);
}


HRESULT __stdcall Device::Present(IDXGISwapChain* This, UINT SyncInterval, UINT Flags)
{
	static bool init = true;
	if (init)
	{
		__asm
		{
			push eax
			mov eax, [ebp + 8]
			mov This, eax
			pop eax
		}

		//GET DEVICE
		This->GetDevice(__uuidof(ID3D11Device), (void**)&m_device);

		//CHECKING IF DEVICE IS VALID
		cout << "New m_pDevice:			0x" << hex << m_device << endl;
		if (!m_device) return false;

		//REPLACING CONTEXT
		m_device->GetImmediateContext(&m_context);

		//CHECKING IF CONTEXT IS VALID
		cout << "New m_context:			0x" << hex << m_context << endl;
		if (!m_context) return false;

		cout << "" << endl;

		m_device->Release();
		// cout << "m_pDevice released" << endl;

		m_context->Release();
		// cout << "m_pContext released" << endl;

		m_swapChain->Release();
		// cout << "m_pSwapChain released" << endl;

		MessageBox(nullptr, "Test wupp wupp", "Caption", MB_OK);

		init = false;
		ID3D11Texture2D *pBackBuffer;
		This->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
		m_device->CreateRenderTargetView(pBackBuffer, NULL, &m_backbuffer);
		pBackBuffer->Release();
	}

	float color[] = { 1.f, 0.f, 0.f, 1.f };
	m_context->ClearRenderTargetView(m_backbuffer, color);
	
	return m_orgPresent(This, SyncInterval, Flags);
}