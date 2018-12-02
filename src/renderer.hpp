#include "d3dhook.hpp"
#include "effect.hpp"
#include "texture.hpp"

#include <d3d.h>
#include <d3d11.h>
#include <cassert>

class Device 
{
public:
	Device() = delete;

	static bool Initialize();

	static HRESULT __stdcall Present(IDXGISwapChain* This, UINT SyncInterval, UINT Flags);
	static void SetOrgPresent(D3D11PresentHook _orgPresent) { m_orgPresent = _orgPresent; }

	static void SetEffect(const Effect& _effect);
	static IDXGISwapChain& GetSwapChain() { assert(m_swapChain); return *m_swapChain; }
private:
	static void Draw();
	static void InitializeParent(IDXGISwapChain* _this);
	
	// d3d11 environment
	static ID3D11Device* m_device;
	static ID3D11DeviceContext* m_context;
	static IDXGISwapChain* m_swapChain;
	static ID3D11RenderTargetView* m_backbuffer;
	
	static const Effect* m_effect;
	static const Texture* m_texture;

	// hook related
	static DWORD_PTR* m_swapChainVtable;
	static D3D11PresentHook m_orgPresent;
};