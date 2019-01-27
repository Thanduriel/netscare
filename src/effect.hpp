#pragma once

#include <algorithm>
#include <d3d.h>
#include <d3d11.h>
#include <DirectXMath.h>


class Effect 
{
public:
	Effect(ID3D11Device* _device, const WCHAR* _vsFilename, const WCHAR* _psFilename);
	// Create Effect from existing pipeline.
	// Decrements the ref count of the given interfaces on destruction.
	Effect(ID3D11VertexShader* _vs, ID3D11PixelShader* _ps, ID3D11InputLayout* _inputLayout,
		ID3D11SamplerState* _sampleState, ID3D11BlendState* _blendState, 
		const DirectX::XMFLOAT4& _factor, UINT _sampleMask);
	Effect();

	~Effect();

	Effect& operator=(Effect&& _effect);

	ID3D11VertexShader* GetVertexShader() const { return m_vertexShader; }
	ID3D11PixelShader* GetPixelShader() const { return m_pixelShader; }
	ID3D11InputLayout* GetInputLayout() const { return m_layout; }
private:
	static HRESULT CompileShader(_In_ LPCWSTR srcFile, _In_ LPCSTR entryPoint, _In_ LPCSTR profile, _Outptr_ ID3DBlob** blob);

	friend void swap(Effect& _a, Effect& _b);

	friend class Device;

	ID3D11VertexShader* m_vertexShader;
	ID3D11PixelShader* m_pixelShader;
	ID3D11InputLayout* m_layout;
	ID3D11SamplerState* m_sampleState;

	// blend state params
	ID3D11BlendState* m_blendState;
	DirectX::XMFLOAT4 m_factor;
	UINT m_sampleMask;
};