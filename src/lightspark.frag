R"(
#ifdef GL_ES
precision highp float;
#endif
uniform sampler2D g_tex1;
uniform sampler2D g_tex2; // mask texture
uniform sampler2D g_tex3; // blend base texture
uniform float yuv;
uniform float alpha;
uniform float direct;
uniform float mask;
uniform float blendMode;
varying vec4 ls_TexCoords[2];
varying vec4 ls_FrontColor;
uniform vec4 colorTransformMultiply;
uniform vec4 colorTransformAdd;
uniform vec4 directColor;

const mat3 YUVtoRGB = mat3(1.164383, 1.164383, 1.164383, //First column
                           0.0,      -.391762, 2.017232, //Second column
                           1.596027, -.812968, 0.0); //Third column

// taken from https://github.com/jamieowen/glsl-blend
float blendOverlay(float base, float blend) {
	return base<0.5?(2.0*base*blend):(1.0-2.0*(1.0-base)*(1.0-blend));
}

vec3 blendOverlay(vec3 base, vec3 blend) {
	return vec3(blendOverlay(base.r,blend.r),blendOverlay(base.g,blend.g),blendOverlay(base.b,blend.b));
}

// algorithm taken from https://www.cairographics.org/operators/
vec4 applyBlendMode(vec4 vsrc, vec4 vdst, vec3 vfactor)
{
	float ares = vsrc.a+vdst.a*(1.0-vsrc.a);
	vec4 vres;
	vres.rgb = (1.0-vdst.a)*vsrc.rgb*vsrc.a + (1.0-vsrc.a)*vdst.rgb*vdst.a + vsrc.a*vdst.a*vfactor/ares;
	vres.a = ares;
	return vres;
}

float invert_value(float value)
{
	float sign_value = sign(value);
	float sign_value_squared = sign_value*sign_value;
	return sign_value_squared / ( value + sign_value_squared - 1.0);
}
void main()
{
	vec4 vbase = texture2D(g_tex1,ls_TexCoords[0].xy);
	// discard everything that doesn't fit the mask
	if (mask != 0.0 && texture2D(g_tex2,ls_TexCoords[1].xy).a == 0.0)
		discard;
#ifdef GL_ES
	vbase.rgb = vbase.bgr;
#endif

	vbase *= alpha;

	// un-premultiply alpha
	vbase.rgb *= invert_value(vbase.a);
	// add colortransformation
	vbase = clamp(vbase*colorTransformMultiply+colorTransformAdd,0.0,1.0);

	vec4 vblenddst = texture2D(g_tex3,ls_TexCoords[1].xy);

	if (blendMode==13.0) {//BLENDMODE_OVERLAY
		vbase = applyBlendMode(vbase,vblenddst,blendOverlay(vblenddst.rgb,vbase.rgb));
	} else if (blendMode==14.0) {//BLENDMODE_HARDLIGHT
		vbase = applyBlendMode(vbase,vblenddst,blendOverlay(vbase.rgb,vblenddst.rgb));
	}
	// premultiply alpha again
	vbase.rgb *= vbase.a;

	//Tranform the value from YUV to RGB
	vec4 val = vbase.bgra - vec4(.062745, 0.501961, 0.501961, 0);
	val.rgb = YUVtoRGB*(val.rgb);

	//Select the right value
	if (direct == 1.0) {
		gl_FragColor = ls_FrontColor;
	} else if (direct == 2.0) {
		if (vbase.a == 0.0)
			discard;
		gl_FragColor.rgb = directColor.rgb*(vbase.rgb);
		gl_FragColor.a = vbase.a;
	} else if (direct == 3.0) {
		gl_FragColor.rgb = directColor.rgb;
		gl_FragColor.a = 1.0;
	} else {
		gl_FragColor = mix(vbase, val, yuv);
	}
}
)"
