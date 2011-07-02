uniform sampler2D g_tex1, g_tex2;
uniform float yuv;
uniform float mask;
uniform float alpha;
varying vec4 ls_TexCoords[2];

const mat3 YUVtoRGB = mat3(	1, 1, 1, //First coloumn
				0, -0.344, 1.772, //Second coloumn
				1.402, -0.714, 0); //Third coloumn

void main()
{
	if(mask==1.0 && texture2D(g_tex2,ls_TexCoords[1].xy).a==0.0)
		discard;

	//Tranform the value from YUV to RGB
	vec4 vbase = texture2D(g_tex1,ls_TexCoords[0].xy);
	vbase *= alpha;
	vec4 val = vbase.bgra-vec4(0,0.5,0.5,0);
	val.rgb = YUVtoRGB*(val.rgb);

	//Select the right value
	gl_FragColor=(vbase*(1.0-yuv))+(val*yuv);
}
