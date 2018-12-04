#include "texture.hpp"

constexpr int VERTEX_COUNT = 4;

Texture::Texture(ID3D11Device* _d3dDevice, float _x1, float _y1, float _x2, float _y2)
{
	// Load the vertex array with data.
	VertexType vertices[VERTEX_COUNT];
	vertices[0].position = D3DXVECTOR3{ _x1, _y1, 0.0f };  // Bottom left.
	vertices[1].position = D3DXVECTOR3{_x1, _y2, 0.0f};  // Top middle.
	vertices[2].position = D3DXVECTOR3{ _x2, _y1, 0.0f };  // Bottom right.
	vertices[3].position = D3DXVECTOR3{ _x2, _y2, 0.0f };  // Bottom right.


	// Set up the description of the static vertex buffer.
	D3D11_BUFFER_DESC vertexBufferDesc;
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(VertexType) * VERTEX_COUNT;
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
	HRESULT result;
	result = _d3dDevice->CreateBuffer(&vertexBufferDesc, &vertexData, &m_vertexBuffer);
	if (FAILED(result))
	{
		MessageBox(nullptr, "failed to create vertex buffer", "Caption", MB_OK);
	}
}

Texture::~Texture()
{
	if (m_vertexBuffer)
	{
		m_vertexBuffer->Release();
		m_vertexBuffer = 0;
	}
}

void Texture::Draw(ID3D11DeviceContext* _deviceContext) const
{
	unsigned int stride;
	unsigned int offset;


	// Set vertex buffer stride and offset.
	stride = sizeof(VertexType);
	offset = 0;

	// Set the vertex buffer to active in the input assembler so it can be rendered.
	_deviceContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);

	// Set the index buffer to active in the input assembler so it can be rendered.
	//_deviceContext->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
	_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	_deviceContext->Draw(VERTEX_COUNT, 0);
}