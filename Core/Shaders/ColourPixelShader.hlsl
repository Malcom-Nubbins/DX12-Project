struct PixelShaderInput
{
	float4 Colour : COLOR;
};

float4 main(PixelShaderInput input) : SV_TARGET
{
	return input.Colour;
}