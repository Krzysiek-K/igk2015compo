
float4x4 ViewProj;
float3 cam_pos;



texture tex;

sampler stex = sampler_state {
	Texture = <tex>;
	MagFilter = Linear;
	MinFilter = Linear;
	MipFilter = Linear;
	AddressU = Clamp;
	AddressV = Clamp;
};




void VS(
	float4 pos			: POSITION,
	float3 normal		: NORMAL,
	out float4 hpos		: POSITION,
	out float3 norm		: NORMAL,
	out float3 _pos		: TEXCOORD0
	)
{
	hpos = mul(pos, ViewProj);
	norm = normal;
	_pos = pos;
}



float4 PS(	float3 normal	: NORMAL,
			float3 pos		: TEXCOORD0 ) : COLOR
{
	float3 vdir = normalize(cam_pos-pos);
	float3 ldir = normalize(float3(-1, -1, 1));
	normal = normalize(normal);

	float3 albedo = tex2D(stex,(pos.yz*float2(1,-1))*.7+.5);


	float3 color = saturate(dot(ldir, normal))*float3(1,.5,.2)*albedo;
	color += (1-color)*float3(.2,.5,1)*(normal.z*.5+.5)*albedo;


	return float4(pow(color, 1/2.2), 1);
}


technique tech {
	pass {
		VertexShader = compile vs_3_0 VS();
		PixelShader = compile ps_3_0 PS();
	}
}
