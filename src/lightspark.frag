#ifdef GL_ES
precision highp float;
#endif
uniform sampler2D g_tex1, g_tex2;
uniform float yuv;
uniform float alpha;
uniform float direct;
varying vec4 ls_TexCoords[2];
varying vec4 ls_FrontColor;

const mat3 YUVtoRGB = mat3(1, 1, 1, //First coloumn
				0, -0.344, 1.772, //Second coloumn
				1.402, -0.714, 0); //Third coloumn

void main()
{
	//Tranform the value from YUV to RGB
	vec4 vbase = texture2D(g_tex1,ls_TexCoords[0].xy);
#ifdef GL_ES
	vbase.rgb = vbase.bgr;
#endif
	vbase *= alpha;
	vec4 val = vbase.bgra-vec4(0,0.5,0.5,0);
	val.rgb = YUVtoRGB*(val.rgb);

	//Select the right value
	if (direct == 1.0) {
		gl_FragColor = ls_FrontColor;
	} else {
		gl_FragColor=(vbase*(1.0-yuv))+(val*yuv);
	}
}
