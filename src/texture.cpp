#include "texture.hpp"
#include <d3d11shader.h>
#include <DDSTextureLoader.h>
#include <string>


constexpr int VERTEX_COUNT = 4;

using namespace DirectX;

Texture::Texture(ID3D11Device* _d3dDevice, WCHAR* _file, float _x1, float _y1, float _x2, float _y2)
{
	HRESULT result;
	result = DirectX::CreateDDSTextureFromFile(_d3dDevice, _file, &m_texture, &m_textureView);
	if (FAILED(result))
	{
		MessageBox(nullptr, "failed to load texture", nullptr, MB_OK);
	}

	// Load the vertex array with data.
	VertexPositionTexture vertices[VERTEX_COUNT];
	vertices[0].position = XMFLOAT3{ _x1, _y1, 0.0f };
	vertices[0].textureCoordinate = XMFLOAT2(0.f, 1.f);
	vertices[1].position = XMFLOAT3{_x1, _y2, 0.0f};
	vertices[1].textureCoordinate = XMFLOAT2(0.f, 0.f);
	vertices[2].position = XMFLOAT3{ _x2, _y1, 0.0f };
	vertices[2].textureCoordinate = XMFLOAT2(1.f, 1.f);
	vertices[3].position = XMFLOAT3{ _x2, _y2, 0.0f };
	vertices[3].textureCoordinate = XMFLOAT2(1.f, 0.f);


	// Set up the description of the static vertex buffer.
	D3D11_BUFFER_DESC vertexBufferDesc;
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(VertexPositionTexture) * VERTEX_COUNT;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the vertex data.
	D3D11_SUBRESOURCE_DATA vertexData;
	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	// Now create the vertex buffer.
	result = _d3dDevice->CreateBuffer(&vertexBufferDesc, &vertexData, &m_vertexBuffer);
	if (FAILED(result))
	{
		MessageBox(nullptr, "failed to create vertex buffer", "Caption", MB_OK);
	}
}

Texture::~Texture()
{
	m_vertexBuffer->Release();
	m_textureView->Release();
	m_texture->Release();
}

void Texture::Draw(ID3D11DeviceContext* _deviceContext) const
{
	unsigned int stride;
	unsigned int offset;

	_deviceContext->PSSetShaderResources(0, 1, &m_textureView);

	// Set vertex buffer stride and offset.
	stride = sizeof(VertexPositionTexture);
	offset = 0;

	// Set the vertex buffer to active in the input assembler so it can be rendered.
	_deviceContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);

	// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
	_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	_deviceContext->Draw(VERTEX_COUNT, 0);
}