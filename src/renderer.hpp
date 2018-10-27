#include "d3dhook.hpp"

#include <d3d.h>
#include <d3d11.h>
#include <cassert>

class Device 
{
public:
	static bool Initialize();

	static HRESULT __stdcall Present(IDXGISwapChain* This, UINT SyncInterval, UINT Flags);

	static IDXGISwapChain& GetSwapChain() { assert(m_swapChain); return *m_swapChain; }
private:
	
	// d3d11 environment
	static ID3D11Device* m_device;
	static ID3D11DeviceContext* m_context;
	static IDXGISwapChain* m_swapChain;

	// hook related
	static DWORD_PTR* m_swapChainVtable;
	static D3D11PresentHook m_orgPresent;
};