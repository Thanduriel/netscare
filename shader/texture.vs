// TYPEDEFS //
struct VertexInputType
{
	float3 position : SV_Position;
	float2 texCoord : TEXCOORD;
};

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD0;
};

PixelInputType VSMain(VertexInputType input)
{
	PixelInputType output;
	
	output.position = float4(input.position,1);
	output.texCoord = input.texCoord;
	
	return output;
}