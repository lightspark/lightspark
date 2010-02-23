uniform vec4 g_rectbase;

void main()
{
	vec4 base=vec4(0,0,0,1);
	base.xy=g_rectbase.xy;

	vec4 base2 = gl_ModelViewProjectionMatrix * base;
	// Transforming The Vertex
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	//gl_Position.xy -= base2.xy;
}
