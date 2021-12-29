cbuffer Scene : register(b0) { float4x4 MVP; };

struct VOut {
    float4 position : SV_POSITION;
    float4 color : COLOR0;
};

VOut VSMain(float3 position : POSITION, float4 color : COLOR0) {
    VOut output;

    output.position = mul(MVP, float4(position.xyz, 1.f));
    output.color    = color;

    return output;
}

float4 PSMain(VOut input) : SV_Target {
    return input.color;
}
