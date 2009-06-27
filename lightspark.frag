uniform sampler1D g_tex;

vec4 solid_color()
{
	return gl_TexCoord[0];
}

vec4 linear_gradient()
{
	float coord=float(gl_FragCoord.x)/1000.0;
	return texture1D(g_tex,coord);
}

void main()
{
	//It's probably faster to compute all this and select the rigth one
	gl_FragData[0]=(solid_color()*gl_Color.r)+
			(linear_gradient()*gl_Color.g);
	gl_FragData[1]=gl_Color;
}
