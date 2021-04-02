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

const mat3 YUVtoRGB = mat3(1, 1, 1, //First coloumn
				0, -0.344, 1.772, //Second coloumn
				1.402, -0.714, 0); //Third coloumn

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
	vec4 val = vbase.bgra-vec4(0,0.5,0.5,0);
	//Tranform the value from YUV to RGB
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
		gl_FragColor=(vbase*(1.0-yuv))+(val*yuv);
	}
}
)"
