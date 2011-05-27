attribute vec4 ls_Color;
attribute vec2 ls_Vertex;
attribute vec2 ls_TexCoord;
uniform vec2 texScale;

void main()
{
	// Transforming The Vertex
	gl_Position=gl_ModelViewProjectionMatrix * vec4(ls_Vertex,0,1);
	gl_FrontColor=ls_Color;
	vec4 t=vec4(0,0,0,1);
	//Position is in normalized screen coords
	t.xy=((gl_Position.xy+vec2(1,1))/2.0)*texScale;
	gl_TexCoord[0]=vec4(ls_TexCoord, 0, 1);
	gl_TexCoord[1]=t;
}
