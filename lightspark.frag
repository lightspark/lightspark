uniform sampler1D m_tex;

void main()
{
	float coord=gl_FragCoord.x/1000;
	gl_FragColor=texture1D(m_tex,coord);
}
