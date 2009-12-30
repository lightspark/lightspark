uniform sampler2D g_tex1;

//We abuse of default varying to pass variou data
//gl_Color: Selector of current shader function
//gl_TexCoord[0]: Fill color/Texture coordinate
//gl_SecondaryColor: interactive id

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
	return texture2D(g_tex1,gl_TexCoord[0].xy);
}

void main()
{
	//It's probably faster to compute all this and select the rigth one
	vec4 ret=(solid_color()*gl_Color.x)+
			(linear_gradient()*gl_Color.y)+
			(tex_lookup()*gl_Color.z);

	gl_FragData[0]=ret;
	//gl_FragData[1]=gl_SecondaryColor*ceil(ret.a);
}
