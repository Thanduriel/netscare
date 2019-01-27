#include "effect.hpp"
#include "renderer.hpp"
#include "utils.hpp"
#include <d3d11shader.h>
#include <d3dcompiler.h>
#include<string>
#include <VertexTypes.h>
#include <CommonStates.h>

#pragma comment(lib,"d3dcompiler.lib")

using namespace std::string_literals;

Effect::Effect(ID3D11Device* _device, const WCHAR* _vsFilename, const WCHAR* _psFilename)
	: m_factor{1.f,1.f,1.f,1.f},
	m_sampleMask(~0)
{
	HRESULT result;
	ID3D10Blob* errorMessage;

	errorMessage = nullptr;
	
	// Compile the vertex shader code.
	Utils::Log().info(L"Compiling vertex shader {}", _vsFilename);
	ID3DBlob *vsBlob = nullptr;
	HRESULT hr = CompileShader(_vsFilename, "VSMain", "vs_4_0_level_9_1", &vsBlob);
	if (FAILED(hr))
	{
	}

	// Compile the pixel shader code.
	Utils::Log().info(L"Compiling pixel shader {0}", _psFilename);
	ID3DBlob *psBlob = nullptr;
	hr = CompileShader(_psFilename, "PSMain", "ps_4_0_level_9_1", &psBlob);
	if (FAILED(hr))
	{
		psBlob->Release();
	}

	// Create the vertex shader from the buffer.
	result = _device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), NULL, &m_vertexShader);
	if (FAILED(result))
	{
		MessageBox(nullptr, "failed to create vs", "Caption", MB_OK);
	}

	// Create the pixel shader from the buffer.
	result = _device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), NULL, &m_pixelShader);
	if (FAILED(result))
	{
		MessageBox(nullptr, "failed to create ps", "Caption", MB_OK);
	}

	const int numElements = DirectX::VertexPositionTexture::InputElementCount;

	// Create the vertex input layout.
	result = _device->CreateInputLayout(DirectX::VertexPositionTexture::InputElements, 
		numElements, vsBlob->GetBufferPointer(),
		vsBlob->GetBufferSize(), &m_layout);
	if (FAILED(result))
	{
		MessageBox(nullptr, "failed to create input layout", std::to_string(numElements).c_str(), MB_OK);
	}

	// Release the vertex shader buffer and pixel shader buffer since they are no longer needed.
	vsBlob->Release();
	psBlob->Release();

	m_sampleState = Device::GetCommonStates()->LinearWrap();
	m_blendState = Device::GetCommonStates()->Additive();
}

Effect::Effect(ID3D11VertexShader* _vs, ID3D11PixelShader* _ps, 
	ID3D11InputLayout* _inputLayout, ID3D11SamplerState* _sampleState, 
	ID3D11BlendState* _blendState, const DirectX::XMFLOAT4& _factor, UINT _sampleMask)
	: m_vertexShader(_vs),
	m_pixelShader(_ps),
	m_layout(_inputLayout),
	m_sampleState(_sampleState),
	m_blendState(_blendState),
	m_factor(_factor),
	m_sampleMask(_sampleMask)
{}

Effect::Effect()
	: m_vertexShader(nullptr),
	m_pixelShader(nullptr),
	m_layout(nullptr),
	m_sampleState(nullptr),
	m_blendState(nullptr)
{}

Effect& Effect::operator=(Effect&& _effect)
{
	swap(*this, _effect);

	return *this;
}

HRESULT Effect::CompileShader(_In_ LPCWSTR srcFile, _In_ LPCSTR entryPoint, _In_ LPCSTR profile, _Outptr_ ID3DBlob** blob)
{
	if (!srcFile || !entryPoint || !profile || !blob)
		return E_INVALIDARG;
	
	*blob = nullptr;

	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
	flags |= D3DCOMPILE_DEBUG;
#endif

	const D3D_SHADER_MACRO defines[] =
	{
		"EXAMPLE_DEFINE", "1",
		NULL, NULL
	};

	ID3DBlob* shaderBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	HRESULT hr = D3DCompileFromFile(srcFile, defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entryPoint, profile,
		flags, 0, &shaderBlob, &errorBlob);
	if (FAILED(hr))
	{
		if (errorBlob)
		{
			MessageBox(nullptr, (char*)errorBlob->GetBufferPointer(), "Caption", MB_OK);
			errorBlob->Release();
		}
		if (shaderBlob)
			shaderBlob->Release();

		return hr;
	}

	*blob = shaderBlob;

	return hr;
}

Effect::~Effect()
{
	if (m_layout)
		m_layout->Release();

	if (m_pixelShader)
		m_pixelShader->Release();

	if (m_vertexShader)
		m_vertexShader->Release();

	if (m_blendState)
		m_blendState->Release();

	if (m_blendState)
		m_blendState->Release();
}

void swap(Effect& _a, Effect& _b)
{
	std::swap(_a.m_vertexShader, _b.m_vertexShader);
	std::swap(_a.m_pixelShader, _b.m_pixelShader);
	std::swap(_a.m_layout, _b.m_layout);
	std::swap(_a.m_sampleState, _b.m_sampleState);
	std::swap(_a.m_blendState, _b.m_blendState);
	std::swap(_a.m_vertexShader, _b.m_vertexShader);
	std::swap(_a.m_factor, _b.m_factor);
	std::swap(_a.m_sampleMask, _b.m_sampleMask);
}

