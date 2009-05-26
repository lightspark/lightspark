uniform sampler1D m_tex;

void main()
{
	float coord=float(gl_FragCoord.x)/1000.0;
	gl_FragColor=texture1D(m_tex,coord);
}
