uniform sampler1D g_tex;
uniform vec2 g_selector;


vec4 solid_color()
{
	return vec4(0.0,0.0,1.0,0.0);
}

vec4 linear_gradient()
{
	float coord=float(gl_FragCoord.x)/1000.0;
	return texture1D(g_tex,coord);
}

void main()
{
	//It's probably faster to compute all this and select the rigth one
	gl_FragColor=(solid_color()*g_selector.x)+
			(linear_gradient()*g_selector.y);
}
