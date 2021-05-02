
Texture2D InputTexture : register(t0);
SamplerState InputSampler : register(s0);

cbuffer constants : register(b0)
{
    float4 ring_color : packoffset(c0);
    float4 ring_bk_color : packoffset(c1);
    float4 inner_circle_color : packoffset(c2);
    float4 bk_color : packoffset(c3);
    float score : packoffset(c4.x);
};

///////////////////////////////////

float isGreaterEqThanZero(float num) { return num >= 0; /*sign(sign(num) + 1)*/ /* sign(sign(num) + 1); */ }

float signOf(float num) { return sign(num) + !num; }//+1 for positives and zero, -1 for negatives

float modulus(float num, float base) { return ((num % base) + base) % base; }

float isLowerEqThanZero(float num) { return num <= 0; /*sign(sign(-num) + 1)*/ /* isGreaterEqThanZero(-num); */ }

float isGreaterEqThan(float a, float b) { return a >= b; /*sign(sign(a - b) + 1)*/ /* isGreaterEqThanZero(a - b); */ }

float isLowerEqThan(float a, float b) { return a <= b; /* sign(sign(-(a - b)) + 1) */ /* isLowerEqThanZero(a - b); */ }

float isGreaterThanZero(float num) { return num > 0;/* !sign(sign(-num)+1) */ /* !isLowerEqThanZero(num) */ }

#define PI 3.1415926535f
#define _2PI (PI * 2.f)
#define stripeRadialStart 0.30f
#define stripeRadialEnd 0.48f
//float stripeDist = (stripeRadialEnd - stripeRadialStart); //NOTE(fran): this is possibly the most insane thing I've ever seen, somehow stripeDist is used/stored/whatever in the wrong way and it breaks the whole algorithm, _but_ if you replace it with '(stripeRadialEnd - stripeRadialStart)' everything works again
//That's why we'll use a macro
#define stripeDist (stripeRadialEnd - stripeRadialStart)

///////////////////////////////////

float makeCircle(float2 absolutePos, float2 center, float radius)
{
	float radialDist = length(absolutePos - center);
	float transparency = pow(.95f + cos(radialDist * (PI / (radius * 2))), 30);

	return isLowerEqThan(radialDist, radius) * saturate(transparency);
}

float4 main_outter_circle(float2 uv) {//1st pass
	float saturation = makeCircle(uv, float2( .5f, .5f ), .5f);
#if 0
	return float4{ isLowerEqThanZero(completion) * .3f, 0, 0, saturation * .8f };
	//TODO(fran): I believe the idea for the red component is that if completion gets to zero it shows the bk in a red-ish color, we could keep that
#else
	float4 res;
	res.rgb = ring_bk_color.rgb;
	res.a = saturation * .8f;
	return res;
#endif
}

float edgeTransparency(float2 absolutePos, float2 circleOrigin, float circleRadius) {
	float2 center = float2( 0.5f, 0.5f );
	float distFromCircleOrigin = length(absolutePos - circleOrigin);

	float2 pos = absolutePos - center;
	float radialDist = length(pos);

	float transparency = pow(cos(distFromCircleOrigin * (PI / stripeDist)) + 0.8f, 10) * isLowerEqThan(distFromCircleOrigin, circleRadius);

	return transparency;
}

float4 main_ring(float2 uv) {//2nd pass
	//Shader taken from https://github.com/ankomas/2019_1C_K3051_BackBuffer/blob/master/TGC.Group/Shaders/Oxygen.fx
	//TODO(fran): all the extra functions this guy added are probably pointless and should be taken out, probably there's some extra perf waiting for us there
	float2 absolutePos = uv;
	float2 center = float2( 0.5f, 0.5f );
	float2 pos = absolutePos - center;
	float radialDist = length(pos);

	//float angleForColor = (PI * isGreaterEqThanZero(pos.y) - signOf(pos.y) * asinf(abs(pos.x) / radialDist));
	float primitiveAngle = (PI * isGreaterEqThanZero(pos.y) - signOf(pos.y) * asin(abs(pos.x) / radialDist));
	float realAngle = modulus(primitiveAngle - isLowerEqThanZero(pos.x) * 2.f * primitiveAngle, _2PI);
	//float blueIntensity = angleForColor / PI;

	float transparency =
		isGreaterEqThan(radialDist, stripeRadialStart)
		* isLowerEqThan(radialDist, stripeRadialEnd)
		* isLowerEqThan(realAngle, score * _2PI)
		* pow(sin((radialDist - stripeRadialStart) * (PI / stripeDist)) + 0.8f, 10);

	float circleDistanceFromOrigin = stripeRadialStart + stripeDist / 2;
	float2 circleOrigin = float2( 0.5f, 0.5f - circleDistanceFromOrigin );
	float2 o2AngleUnitVector = float2( sin(score * _2PI), -cos(score * _2PI) );
	float2 secondCircleOrigin = center + o2AngleUnitVector * circleDistanceFromOrigin;

	float circleRadius = stripeDist / 2;

	float lowCircleTransparency = edgeTransparency(absolutePos, circleOrigin, circleRadius);
	float highCircleTransparency = edgeTransparency(absolutePos, secondCircleOrigin, circleRadius);

	float finalTransparency = max(transparency, max(lowCircleTransparency, highCircleTransparency));

	//return float4( 1.f - score, score, blueIntensity, score * finalTransparency );
	//res.a = score * finalTransparency;
	return float4(ring_color.rgb, isGreaterThanZero(score) * finalTransparency);
}

float4 main_inner_circle(float2 uv) {//3rd pass
	float2 position = uv;
	float2 center = float2( .5f, .5f );
	float radius = .29f;
	float saturation = makeCircle(position, center, radius);
#if 0
	float3 color = float3{ 0.1 + isLowerEqThanZero(oxygen) * 0.3 * 0.5, 0.2, 0.2 };
	//TODO(fran): same idea as in main_outter_circle here
	float light = 1 - (position.y - center.x - radius) / (radius * 2);
#else
	float4 color = inner_circle_color;
	float light = 1;//NOTE: we want a flat look
#endif

	float4 res;
	res.rgb = color.rgb * light;
	res.a = saturation;
	return res;
}

///////////////////////////////////

bool is_in_circle(float2 uv) {
	float rotation = (score * _2PI);
	float big_radius = .25f;
	float2 center = float2(.5f,.5f) + big_radius * float2(cos(rotation),sin(rotation));
	float radius = stripeDist;
	return pow(uv.x - center.x, 2) + pow(uv.y - center.y, 2) <= pow(radius,2);
}

float4 main(
	float4 clipSpaceOutput  : SV_POSITION,
	float4 sceneSpaceOutput : SCENE_POSITION,
	float4 texelSpaceInput0 : TEXCOORD0
) : SV_Target
{
	// Samples pixel from ten pixels above current position.
	//float2 sampleLocation =
	//    texelSpaceInput0.xy    // Sample position for the current output pixel.
	//    + float2(0,-10)        // An offset from which to sample the input, specified in pixels.
	//    * texelSpaceInput0.zw; // Multiplier that converts pixel offset to sample position offset.
	//float4 color = InputTexture.Sample(
	//    InputSampler,          // Sampler and Texture must match for a given input.
	//    sampleLocation
	//    );

	float4 color = bk_color;

	float4 pass1 = main_outter_circle(texelSpaceInput0.xy);
	pass1 = saturate(pass1);
	pass1.rgb *= pass1.a;

	float4 pass2 = main_ring(texelSpaceInput0.xy);
	pass2 = saturate(pass2);
	pass2.rgb *= pass2.a;

	float4 pass3 = main_inner_circle(texelSpaceInput0.xy);
	pass3 = saturate(pass3);
	pass3.rgb *= pass3.a;

	color = (1.f - pass1.a) * color + pass1;
	color = (1.f - pass2.a) * color + pass2;
	color = (1.f - pass3.a) * color + pass3;

	color.rgb = sqrt(color.rgb);//TODO(fran): I gotta do this since I square the values when sending them to the GPU to correct for gamma, see if this really has an effect on the output, otherwise remove squaring from input (COLORREF_to_v4_linear1) and sqrt from here

	return color;
}