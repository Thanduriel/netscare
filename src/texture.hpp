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
	Texture(ID3D11Device* _d3dDevice, const WCHAR* _file, float _x1, float _y1, 
		float _scaleX = 1.f, float _scaleY = 1.f);
	Texture(ID3D11Device* _d3dDevice, const uint8_t* _data, size_t _dataSize, float _x1, float _y1,
		float _scaleX = 1.f, float _scaleY = 1.f);
	~Texture();

	void Draw(ID3D11DeviceContext* _deviceContext) const;
private:
	void CreateBuffer(ID3D11Device* _d3dDevice, float _x1, float _y1, float _scaleX, float _scaleY);

	ID3D11Buffer* m_vertexBuffer;
	ID3D11Resource* m_texture;
	ID3D11ShaderResourceView* m_textureView;
};