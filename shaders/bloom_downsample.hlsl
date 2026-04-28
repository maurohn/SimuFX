// ============================================================================
// bloom_downsample.hlsl — 13-tap downsampling with threshold + soft knee
// Constant registers:
//   c5: Threshold, Intensity, Radius, SoftKnee
//   c6: RcpW, RcpH, 0, 0
// ============================================================================

sampler2D ScreenTex : register(s0);

float4 c5 : register(c5);
float4 c6 : register(c6);

#define THRESHOLD c5.x
#define INTENSITY c5.y
#define SOFTKNEE  c5.w
#define RCP_W     c6.x
#define RCP_H     c6.y

float3 SoftKneeThreshold(float3 col, float threshold, float knee)
{
    float brightness = max(col.r, max(col.g, col.b));
    float rq = clamp(brightness - threshold + knee, 0.0f, 2.0f * knee);
    rq = (rq * rq) / (4.0f * knee + 0.00001f);
    float weight = max(rq, brightness - threshold) / max(brightness, 0.00001f);
    return col * weight;
}

float4 PS_BloomDown(float2 uv : TEXCOORD0) : COLOR
{
    float2 px = float2(RCP_W, RCP_H);

    // 13-tap Kawase-inspired downsample
    float3 col  = tex2D(ScreenTex, uv).rgb * 4.0f;
    col += tex2D(ScreenTex, uv + float2(-1,-1)*px).rgb;
    col += tex2D(ScreenTex, uv + float2( 1,-1)*px).rgb;
    col += tex2D(ScreenTex, uv + float2(-1, 1)*px).rgb;
    col += tex2D(ScreenTex, uv + float2( 1, 1)*px).rgb;
    col += tex2D(ScreenTex, uv + float2(-2,-2)*px*0.5f).rgb * 0.5f;
    col += tex2D(ScreenTex, uv + float2( 2,-2)*px*0.5f).rgb * 0.5f;
    col += tex2D(ScreenTex, uv + float2(-2, 2)*px*0.5f).rgb * 0.5f;
    col += tex2D(ScreenTex, uv + float2( 2, 2)*px*0.5f).rgb * 0.5f;
    col /= 8.0f;

    // Threshold filter with soft knee
    col = SoftKneeThreshold(col, THRESHOLD, SOFTKNEE);

    return float4(col, 1.0f);
}
