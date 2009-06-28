uniform sampler2D g_tex1;
uniform sampler2D g_tex2;

vec4 solid_color()
{
	return gl_TexCoord[0];
}

vec4 linear_gradient()
{
	vec2 coord=vec2(float(gl_FragCoord.x)/1000.0,0);
	return texture2D(g_tex1,coord);
}

vec4 tex_lookup()
{
	return texture2D(g_tex2,gl_TexCoord[0].xy);
}

void main()
{
	//It's probably faster to compute all this and select the rigth one
	gl_FragData[0]=(solid_color()*gl_Color.x)+
			(linear_gradient()*gl_Color.y)+
			(tex_lookup()*gl_Color.z);
	gl_FragData[1]=gl_Color;
}
