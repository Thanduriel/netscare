#include "d3dhook.hpp"
#include "effect.hpp"
#include "texture.hpp"
#include "PipeServer.hpp"

#include <DirectXMath.h>
#include <d3d.h>
#include <d3d11.h>
#include <cassert>
#include <CommonStates.h>
#include <filesystem>

class Device 
{
public:
	Device() = delete;

	static bool Initialize();

	static HRESULT __stdcall Present(IDXGISwapChain* This, UINT SyncInterval, UINT Flags);
	static void SetOrgPresent(D3D11PresentHook _orgPresent) { m_orgPresent = _orgPresent; }

	static void SetEffect(const Effect& _effect);
	static IDXGISwapChain& GetSwapChain() { assert(m_swapChain); return *m_swapChain; }

	static const DirectX::CommonStates* GetCommonStates() { return m_commonStates; }

	static void SetResourcePath(const std::filesystem::path& _path) { m_resourcePath = _path; }
	static void SetHWND(HWND _handle) { m_windowHandle = _handle; }
	static void SetPipeNode(const PipeNode& node) { m_pipeNode = node; };

	static DirectX::XMINT2 GetBufferSize();
private:
	static void Draw();
	static void InitializeParent(IDXGISwapChain* _this);
	// save the current shader pipline so that it can be restored later
	static void SaveCurrentState();

	// d3d11 environment
	static ID3D11Device* m_device;
	static ID3D11DeviceContext* m_context;
	static IDXGISwapChain* m_swapChain;
	static ID3D11RenderTargetView* m_backbuffer;
	
	static const DirectX::CommonStates* m_commonStates;
	static const Effect* m_effect;
	static const Texture* m_texture;
	static Effect m_previousEffect;
	struct TextureState
	{
		D3D11_PRIMITIVE_TOPOLOGY primitiveTopology;
		ID3D11Buffer* buffer;
		ID3D11ShaderResourceView* resourceView;
		unsigned stride;
		unsigned offset;
	};
	static TextureState m_previousTexState;

	static HWND m_windowHandle;
	static std::filesystem::path m_resourcePath;

	// hook related
	static DWORD_PTR* m_swapChainVtable;
	static D3D11PresentHook m_orgPresent;

	// Pipes
	static PipeNode m_pipeNode;
};