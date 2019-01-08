#pragma once

#include <d3d.h>
#include <d3d11.h>


class Effect 
{
public:
	Effect(ID3D11Device* _device, WCHAR* _vsFilename, WCHAR* _psFilename);
	~Effect();

	ID3D11VertexShader* GetVertexShader() const { return m_vertexShader; }
	ID3D11PixelShader* GetPixelShader() const { return m_pixelShader; }
	ID3D11InputLayout* GetInputLayout() const { return m_layout; }
private:
	static HRESULT CompileShader(_In_ LPCWSTR srcFile, _In_ LPCSTR entryPoint, _In_ LPCSTR profile, _Outptr_ ID3DBlob** blob);

	friend class Device;

	ID3D11VertexShader* m_vertexShader;
	ID3D11PixelShader* m_pixelShader;
	ID3D11InputLayout* m_layout;
	ID3D11SamplerState* m_sampleState;
	ID3D11BlendState* m_blendState;
};