uniform sampler2D g_tex1;
uniform vec2 texScale;

//We abuse of default varying to pass variou data
//gl_Color: Selector of current shader function
//gl_TexCoord[0]: Fill color/Texture coordinate

const mat3 YUVtoRGB = mat3(	1, 1, 1, //First coloumn
				0, -0.344, 1.772, //Second coloumn
				1.402, -0.714, 0); //Third coloumn

vec4 solid_color()
{
	return gl_TexCoord[0];
}

vec4 linear_gradient()
{
	//vec2 coord=vec2(float(gl_FragCoord.x)/1000.0,0);
	//return texture2D(g_tex1,coord);
	//TODO: not working, accessing gl_FragCoord causes access violation
	
	return vec4(0,0,1,1);
}

vec4 tex_lookup()
{
	return texture2D(g_tex1,gl_TexCoord[0].xy*texScale);
}

vec4 tex_lookup_yuv()
{
	//Pixel format is VUYA
	vec4 val=texture2D(g_tex1,gl_TexCoord[0].xy*texScale).bgra-vec4(0,0.5,0.5,0);
	val.rgb=YUVtoRGB*(val.rgb);
	return val;
}

void main()
{
	gl_FragColor=(solid_color()*gl_Color.x)+
			(linear_gradient()*gl_Color.y)+
			(tex_lookup()*gl_Color.z)+
			(tex_lookup_yuv()*gl_Color.w);
}
