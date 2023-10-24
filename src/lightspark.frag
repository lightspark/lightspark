R"(
#ifdef GL_ES
precision highp float;
#endif
uniform sampler2D g_tex1;
uniform sampler2D g_tex2; // mask texture
uniform sampler2D g_tex3; // blend base texture
uniform sampler2D g_tex4; // filter original rendered displayobject texture
uniform sampler2D g_tex5; // previous filter output texture
uniform float yuv;
uniform float alpha;
uniform float direct;
uniform float mask;
uniform float isFirstFilter;
uniform float blendMode;
varying vec4 ls_TexCoords[2];
varying vec4 ls_FrontColor;
uniform vec4 colorTransformMultiply;
uniform vec4 colorTransformAdd;
uniform vec4 directColor;
uniform float filterdata[256]; // FILTERDATA_MAXSIZE
uniform vec4 gradientcolors[256];

const mat3 YUVtoRGB = mat3(1.164383, 1.164383, 1.164383, //First column
                           0.0,      -.391762, 2.017232, //Second column
                           1.596027, -.812968, 0.0); //Third column

// taken from https://github.com/jamieowen/glsl-blend
float blendOverlay(float base, float blend) {
	return base<0.5?(2.0*base*blend):(1.0-2.0*(1.0-base)*(1.0-blend));
}

vec3 blendOverlay(vec3 base, vec3 blend) {
	return vec3(blendOverlay(base.r,blend.r),blendOverlay(base.g,blend.g),blendOverlay(base.b,blend.b));
}

// algorithm taken from https://www.cairographics.org/operators/
vec4 applyBlendMode(vec4 vsrc, vec4 vdst, vec3 vfactor)
{
	float ares = vsrc.a+vdst.a*(1.0-vsrc.a);
	vec4 vres;
	vres.rgb = (1.0-vdst.a)*vsrc.rgb*vsrc.a + (1.0-vsrc.a)*vdst.rgb*vdst.a + vsrc.a*vdst.a*vfactor/ares;
	vres.a = ares;
	return vres;
}

float invert_value(float value)
{
	float sign_value = sign(value);
	float sign_value_squared = sign_value*sign_value;
	return sign_value_squared / ( value + sign_value_squared - 1.0);
}

// filter methods
// TODO all these methods are more or less just taken from their c++ implementation in applyFilter() so they don't take much advantage of vector arithmetics etc.
vec4 getDstPx(vec2 pos)
{
	return isFirstFilter != 0.0 ? texture2D(g_tex4, pos) : texture2D(g_tex5, pos);
}

vec4 filter_blur()
{
	vec4 sum = vec4(0.0);
	float blurx = filterdata[1]+0.5;
	float blury = filterdata[2]+0.5;
	float width = filterdata[3];
	float height = filterdata[4];
	for (float i = 0.0; i < blurx; ++i)
	{
		for (float j = 0.0; j < blury; ++j)
		{
			sum += texture2D(g_tex1,ls_TexCoords[0].xy+vec2( (i-blurx/2.0)/width, (j-blury/2.0)/height));
		}
	}
	return sum/vec4(blurx*blury);
}
vec4 filter_dropshadow(float inner, float knockout, vec4 color, float strength, vec2 startpos)
{
	float glowalpha = inner == 1.0 ? 1.0-texture2D(g_tex1,ls_TexCoords[0].xy+startpos).a : texture2D(g_tex1,ls_TexCoords[0].xy+startpos).a;
	float srcalpha = glowalpha*color.a*strength;
	vec4 dst = getDstPx(ls_TexCoords[0].xy);
	float dstalpha = dst.a;

	if (inner==1.0) 
	{
		if (knockout==1.0) 
			return vec4(clamp(color.r*srcalpha*dstalpha,0.0,1.0),
						clamp(color.g*srcalpha*dstalpha,0.0,1.0),
						clamp(color.b*srcalpha*dstalpha,0.0,1.0),
						clamp(srcalpha*dstalpha,0.0,1.0));
		else
			return vec4(clamp(color.r*srcalpha*dstalpha+dst.r*(1.0-srcalpha),0.0,1.0),
						clamp(color.g*srcalpha*dstalpha+dst.g*(1.0-srcalpha),0.0,1.0),
						clamp(color.b*srcalpha*dstalpha+dst.b*(1.0-srcalpha),0.0,1.0),
						clamp(srcalpha*dstalpha+dstalpha*(1.0-srcalpha),0.0,1.0));
	}
	else
	{
		if (knockout==1.0)
			return vec4(clamp(color.r*srcalpha*(1.0-dstalpha),0.0,1.0),
						clamp(color.g*srcalpha*(1.0-dstalpha),0.0,1.0),
						clamp(color.b*srcalpha*(1.0-dstalpha),0.0,1.0),
						clamp(srcalpha*(1.0-dstalpha),0.0,1.0));
		else
			return vec4(clamp(color.r*srcalpha*(1.0-dstalpha)+dst.r,0.0,1.0),
						clamp(color.g*srcalpha*(1.0-dstalpha)+dst.g,0.0,1.0),
						clamp(color.b*srcalpha*(1.0-dstalpha)+dst.b,0.0,1.0),
						clamp(srcalpha*(1.0-dstalpha)+dstalpha,0.0,1.0));
	}
}

vec4 filter_bevel()
{
	float inner = filterdata[1];
	float knockout = filterdata[2];
	float strength = filterdata[3];
	vec2 highlightOffset = vec2(-filterdata[4],filterdata[5]);
	vec2 shadowOffset = vec2(-filterdata[6],filterdata[7]);
	float alphahigh = filter_dropshadow(inner,1.0,vec4(1.0,1.0,1.0,1.0),1.0,highlightOffset).a * strength * 256.0;
	float alphashadow = filter_dropshadow(inner,1.0,vec4(1.0,1.0,1.0,1.0),1.0,shadowOffset).a * strength * 256.0;
	int gradientindex = 128+clamp(int(alphahigh - alphashadow)/2,-128,127);
	vec4 combinedpixel = gradientcolors[gradientindex];
	vec4 dst = getDstPx(ls_TexCoords[0].xy);

	if (inner==1.0)
	{
		combinedpixel.rgb *= combinedpixel.a;
		if (knockout==1.0)
			return vec4(combinedpixel.r,
						combinedpixel.g,
						combinedpixel.b,
						clamp (combinedpixel.a*dst.a,0.0,1.0));
		else
			return vec4(clamp (dst.r*(1.0-gradientcolors[gradientindex].a)+combinedpixel.r,0.0,1.0),
						clamp (dst.g*(1.0-gradientcolors[gradientindex].a)+combinedpixel.g,0.0,1.0),
						clamp (dst.b*(1.0-gradientcolors[gradientindex].a)+combinedpixel.b,0.0,1.0),
						dst.a);
	}
	else
	{
		if (knockout==1.0)
			return vec4(combinedpixel.r,
						combinedpixel.g,
						combinedpixel.b,
						clamp (combinedpixel.a*(1.0-dst.a),0.0,1.0));
		else
			return vec4(clamp (combinedpixel.r*(1.0-dst.r)*dst.a+dst.r,0.0,1.0),
						clamp (combinedpixel.g*(1.0-dst.g)*dst.a+dst.g,0.0,1.0),
						clamp (combinedpixel.b*(1.0-dst.b)*dst.a+dst.b,0.0,1.0),
						clamp (combinedpixel.a*(1.0-dst.a)*dst.a+dst.a,0.0,1.0));
	}
}

vec4 filter_gradientglow()
{
	float inner = filterdata[1];
	float knockout = filterdata[2];
	float strength = filterdata[3];
	vec2 startpos = vec2(filterdata[4],filterdata[5]);
	float glowalpha = inner == 1.0 ? 1.0-texture2D(g_tex1,ls_TexCoords[0].xy+startpos).a : texture2D(g_tex1,ls_TexCoords[0].xy+startpos).a;
	vec4 color = gradientcolors[int(glowalpha*256.0)];
	return filter_dropshadow(inner, knockout,color , strength, startpos);
}

vec4 filter_colormatrix()
{
	vec4 src = texture2D(g_tex1,ls_TexCoords[0].xy);
	src.rgb *= src.a;
	float alpha = filterdata[16]*src.r + (filterdata[17]*src.g) + (filterdata[18]*src.b) + (filterdata[19]*src.a) + filterdata[20];
	return vec4(clamp ((filterdata[ 1]*src.r + (filterdata[ 2]*src.g) + (filterdata[ 3]*src.b) + (filterdata[ 4]*src.a) + filterdata[ 5]/255.0)*alpha, 0.0, 1.0),
				clamp ((filterdata[ 6]*src.r + (filterdata[ 7]*src.g) + (filterdata[ 8]*src.b) + (filterdata[ 9]*src.a) + filterdata[10]/255.0)*alpha, 0.0, 1.0),
				clamp ((filterdata[11]*src.r + (filterdata[12]*src.g) + (filterdata[13]*src.b) + (filterdata[14]*src.a) + filterdata[15]/255.0)*alpha, 0.0, 1.0),
				clamp (alpha, 0.0, 1.0));
}

vec4 filter_convolution()
{
	// spec is not really clear how this should be implemented, especially when using something different than a 3x3 matrix:
	// "
	// For a 3 x 3 matrix convolution, the following formula is used for each independent color channel:
	// dst (x, y) = ((src (x-1, y-1) * a0 + src(x, y-1) * a1....
	//					  src(x, y+1) * a7 + src (x+1,y+1) * a8) / divisor) + bias
	// "
	float bias = filterdata[1];
	float _clamp = filterdata[2];
	float divisor = filterdata[3];
	float preserveAlpha = filterdata[4];
	vec4 color = vec4(filterdata[5],filterdata[6],filterdata[7],filterdata[8]);
	float width=filterdata[9];
	float height=filterdata[10];
	float mX=filterdata[11];
	float mY=filterdata[12];

	vec2 start = ls_TexCoords[0].xy;
	vec4 src = texture2D(g_tex1,ls_TexCoords[0].xy);

	float redResult   = 0.0;
	float greenResult = 0.0;
	float blueResult  = 0.0;
	float alphaResult = 0.0;
	for (float y=0.0; y <mY; y++)
	{
		for (float x=0.0; x <mX; x++)
		{
			float data = filterdata[13+int(y*mX+x)];
			if (
					(start.x <= mX/2.0
					|| start.x >= (width-mX/2.0)
					|| start.y <= mY/2.0
					|| start.y >= height-mY/2.0))
			{
				if (_clamp==1.0)
				{
					alphaResult += src.a*data;
					redResult   += src.r*data;
					greenResult += src.g*data;
					blueResult  += src.b*data;
				}
				else
				{
					alphaResult += color.a*data;
					redResult   += color.r*data;
					greenResult += color.g*data;
					blueResult  += color.b*data;
				}
			}
			else
			{
				vec4 dst = texture2D(g_tex1,ls_TexCoords[0].xy+vec2((x-mX/2.0)/width,(y-mY/2.0)/height));
				alphaResult += dst.a*data;
				redResult   += dst.r*data;
				greenResult += dst.g*data;
				blueResult  += dst.b*data;
			}
		}
	}
	return vec4(clamp(redResult   / divisor + bias,0.0,1.0),
				clamp(greenResult / divisor + bias,0.0,1.0),
				clamp(blueResult  / divisor + bias,0.0,1.0),
				preserveAlpha == 1.0 ? src.a : clamp(alphaResult / divisor + bias,0.0,1.0));
}

void main()
{
	vec4 vbase = texture2D(g_tex1,ls_TexCoords[0].xy);
	// discard everything that doesn't fit the mask
	if (mask != 0.0 && texture2D(g_tex2,ls_TexCoords[1].xy).a == 0.0)
		discard;
#ifdef GL_ES
	vbase.rgb = vbase.bgr;
#endif
	// apply filter
	if (filterdata[0] > 0.0) {
		if (filterdata[0]==1.0) {//FILTERSTEP_BLUR
			vbase = filter_blur();
		} else if (filterdata[0]==2.0) {// FILTERSTEP_DROPSHADOW
			vbase = filter_dropshadow(filterdata[1],filterdata[2],vec4(filterdata[4],filterdata[5],filterdata[6],filterdata[7]),filterdata[3],vec2(filterdata[8],filterdata[9]));
		} else if (filterdata[0]==3.0) {// FILTERSTEP_GRADIENT_GLOW
			vbase = filter_gradientglow();
		} else if (filterdata[0]==4.0) {// FILTERSTEP_BEVEL
			vbase = filter_bevel();
		} else if (filterdata[0]==5.0) {// FILTERSTEP_COLORMATRIX
			vbase = filter_colormatrix();
		} else if (filterdata[0]==6.0) {// FILTERSTEP_CONVOLUTION
			vbase = filter_colormatrix();
		}
	}

	vbase *= alpha;

	// un-premultiply alpha
	vbase.rgb *= invert_value(vbase.a);
	// add colortransformation
	vbase = clamp(vbase*colorTransformMultiply+colorTransformAdd,0.0,1.0);

	vec4 vblenddst = texture2D(g_tex3,ls_TexCoords[1].xy);

	if (blendMode==13.0) {//BLENDMODE_OVERLAY
		vbase = applyBlendMode(vbase,vblenddst,blendOverlay(vblenddst.rgb,vbase.rgb));
	} else if (blendMode==14.0) {//BLENDMODE_HARDLIGHT
		vbase = applyBlendMode(vbase,vblenddst,blendOverlay(vbase.rgb,vblenddst.rgb));
	}
	// premultiply alpha again
	vbase.rgb *= vbase.a;

	//Tranform the value from YUV to RGB
	vec4 val = vbase.bgra - vec4(.062745, 0.501961, 0.501961, 0);
	val.rgb = YUVtoRGB*(val.rgb);

	//Select the right value
	if (direct == 1.0) {
		gl_FragColor = ls_FrontColor;
	} else if (direct == 2.0) {
		if (vbase.a == 0.0)
			discard;
		gl_FragColor.rgb = directColor.rgb*(vbase.rgb);
		gl_FragColor.a = vbase.a;
	} else if (direct == 3.0) {
		gl_FragColor.rgb = directColor.rgb;
		gl_FragColor.a = 1.0;
	} else {
		gl_FragColor = mix(vbase, val, yuv);
	}
}
)"
