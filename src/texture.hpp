#pragma once

#include <d3d.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include "VertexTypes.h"

class Texture
{
public:
	// p1 lower left
	// p2 upper right
	Texture(ID3D11Device* _d3dDevice, WCHAR* _file, float _x1, float _y1, 
		float _scaleX = 1.f, float _scaleY = 1.f);
	~Texture();

	void Draw(ID3D11DeviceContext* _deviceContext) const;
private:

	ID3D11Buffer* m_vertexBuffer;
	ID3D11Resource* m_texture;
	ID3D11ShaderResourceView* m_textureView;
};