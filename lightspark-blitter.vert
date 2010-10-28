uniform vec2 texScale;

void main()
{
	// Transforming The Vertex
	gl_Position=ftransform();
	gl_FrontColor=vec4(1,0,0,1);
	vec4 t=vec4(0,0,0,1);
	//Position is in normalized screen coords
	t.xy=((gl_Position.xy+vec2(1,1))/2.0)*texScale;
	gl_TexCoord[0]=t;
}
