// TYPEDEFS //
struct VertexInputType
{
	float3 position : POSITION;
};

struct PixelInputType
{
	float4 position : SV_POSITION;
};

PixelInputType VSMain(VertexInputType input)
{
	PixelInputType output;
	
	output.position = float4(input.position,1);
	
	return output;
}