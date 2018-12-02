#pragma once

#include <d3d.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

class Texture
{
public:
	Texture(ID3D11Device* _d3dDevice);
	~Texture();

	void Draw(ID3D11DeviceContext* _deviceContext) const;
private:
	struct D3DXVECTOR3
	{
		FLOAT x;
		FLOAT y;
		FLOAT z;
	};

	struct D3DXVECTOR4
	{
		FLOAT r;
		FLOAT g;
		FLOAT b;
		FLOAT a;
	};

	struct VertexType
	{
		D3DXVECTOR3 position;
		D3DXVECTOR4 color;
	};

	ID3D11Buffer* m_vertexBuffer;
	ID3D11Buffer* m_indexBuffer;
};