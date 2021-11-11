R"(
#ifdef GL_ES
precision highp float;
#endif
uniform sampler2D g_tex1;
uniform sampler2D g_tex2;
uniform float yuv;
uniform float alpha;
uniform float direct;
uniform float mask;
varying vec4 ls_TexCoords[2];
varying vec4 ls_FrontColor;
uniform vec4 colorTransformMultiply;
uniform vec4 colorTransformAdd;
uniform vec4 directColor;

const mat3 YUVtoRGB = mat3(1.164383, 1.164383, 1.164383, //First column
                           0.0,      -.391762, 2.017232, //Second column
                           1.596027, -.812968, 0.0); //Third column

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
	// add colortransformation
	if (colorTransformMultiply != vec4(1,1,1,1) || colorTransformAdd != vec4(0,0,0,0))
	{
		vbase = max(min(vbase*colorTransformMultiply+colorTransformAdd,1.0),0.0);
		// premultiply alpha as it may have changed in colorTramsform
		vbase.rgb *= vbase.a;
	}

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
