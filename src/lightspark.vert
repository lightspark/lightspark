R"(
attribute vec4 ls_Color;
attribute vec2 ls_Vertex;
attribute vec2 ls_TexCoord;
uniform mat4 ls_ProjectionMatrix;
uniform mat4 ls_ModelViewMatrix;
varying vec4 ls_TexCoords[2];
varying vec4 ls_FrontColor;

void main()
{
	// Transforming The Vertex
	vec2 st = ls_Vertex;
	gl_Position=ls_ProjectionMatrix * ls_ModelViewMatrix * vec4(st,0,1);
	ls_FrontColor=ls_Color;

	vec4 t = vec4(0,0,0,1);

	// Position is in normalized screen coords
	t.xy = ((gl_Position.xy + vec2(1,1)) / 2.0);
	ls_TexCoords[0] = vec4(ls_TexCoord, 0, 1);
	ls_TexCoords[1] = t;
}
)"
