R"(
attribute vec4 ls_Color;
attribute vec2 ls_Vertex;
attribute vec2 ls_TexCoord;
uniform mat4 ls_ProjectionMatrix;
uniform mat4 ls_ModelViewMatrix;
uniform vec2 texScale;
varying vec4 ls_TexCoords[2];
varying vec4 ls_FrontColor;
uniform float rotation;
uniform vec2 beforeRotate;
uniform vec2 afterRotate;
uniform vec2 startPosition;
uniform vec2 scale;

mat2 rotate2d(float _angle){
	return mat2(cos(_angle),-sin(_angle),
		sin(_angle),cos(_angle));
}
void main()
{
	// Transforming The Vertex
	vec2 st = ls_Vertex;
	st -= beforeRotate;
	st *= scale;
	st *= rotate2d( rotation );
	st += afterRotate;
	st += startPosition;
	gl_Position=ls_ProjectionMatrix * ls_ModelViewMatrix * vec4(st,0,1);
	ls_FrontColor=ls_Color;
	vec4 t=vec4(0,0,0,1);

	//Position is in normalized screen coords
	t.xy=((gl_Position.xy+vec2(1,1))/2.0);//*texScale;
	ls_TexCoords[0]=vec4(ls_TexCoord, 0, 1);
	ls_TexCoords[1]=t;
}
)"
