#include "effect.hpp"
#include <d3d11shader.h>
#include <d3dcompiler.h>
#include<string>

#pragma comment(lib,"d3dcompiler.lib")

Effect::Effect(ID3D11Device* _device, WCHAR* _vsFilename, WCHAR* _psFilename)
{
	HRESULT result;
	ID3D10Blob* errorMessage;
	D3D11_INPUT_ELEMENT_DESC polygonLayout[2];
	unsigned int numElements;


	errorMessage = nullptr;
	
	// Compile the vertex shader code.
	ID3DBlob *vsBlob = nullptr;
	HRESULT hr = CompileShader(_vsFilename, "VSMain", "vs_4_0_level_9_1", &vsBlob);
	if (FAILED(hr))
	{
	}

	// Compile the pixel shader code.
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

	// Now setup the layout of the data that goes into the shader.
	// This setup needs to match the VertexType stucture in the ModelClass and in the shader.
	polygonLayout[0].SemanticName = "POSITION";
	polygonLayout[0].SemanticIndex = 0;
	polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[0].InputSlot = 0;
	polygonLayout[0].AlignedByteOffset = 0;
	polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[0].InstanceDataStepRate = 0;
	
	polygonLayout[1].SemanticName = "COLOR";
	polygonLayout[1].SemanticIndex = 0;
	polygonLayout[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	polygonLayout[1].InputSlot = 0;
	polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[1].InstanceDataStepRate = 0;
	
	// Get a count of the elements in the layout.
	numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);

	// Create the vertex input layout.
	result = _device->CreateInputLayout(polygonLayout, numElements, vsBlob->GetBufferPointer(),
		vsBlob->GetBufferSize(), &m_layout);
	if (FAILED(result))
	{
		MessageBox(nullptr, "failed to create input layout", std::to_string(numElements).c_str(), MB_OK);
	}

	// Release the vertex shader buffer and pixel shader buffer since they are no longer needed.
	vsBlob->Release();
	psBlob->Release();
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
}
