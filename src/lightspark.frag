uniform sampler2D g_tex1, g_tex2;
uniform vec2 texScale;

//We abuse of default varying to pass a selector
//gl_Color: Selector of current shader function

const mat3 YUVtoRGB = mat3(	1, 1, 1, //First coloumn
				0, -0.344, 1.772, //Second coloumn
				1.402, -0.714, 0); //Third coloumn

vec4 tex_lookup()
{
	return texture2D(g_tex1,gl_TexCoord[0].xy);
}

vec4 tex_lookup_yuv()
{
	//Pixel format is VUYA
	vec4 val=texture2D(g_tex1,gl_TexCoord[0].xy).bgra-vec4(0,0.5,0.5,0);
	val.rgb=YUVtoRGB*(val.rgb);
	return val;
}

void main()
{
	if(gl_Color.x==1.0 && texture2D(g_tex2,gl_TexCoord[1].xy).a==0.0)
		discard;

	gl_FragColor=(tex_lookup()*gl_Color.z)+
			(tex_lookup_yuv()*gl_Color.w);
}
