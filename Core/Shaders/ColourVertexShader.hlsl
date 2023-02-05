struct ModelViewProjection
{
	matrix MVP;
};

ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);

struct VertexPosColour
{
	float3 Position : POSITION;
	float3 Colour : COLOR;
};

struct VertexShaderOutput
{
	float4 Colour : COLOR;
	float4 SV_Pos : SV_Position;
};

VertexShaderOutput main(VertexPosColour inVertex)
{
	VertexShaderOutput vertexOutput;
	vertexOutput.SV_Pos = mul(ModelViewProjectionCB.MVP, float4(inVertex.Position, 1.0f));
	vertexOutput.Colour = float4(inVertex.Colour, 1.0f);
	
	return vertexOutput;
}