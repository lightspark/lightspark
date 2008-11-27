#include "tags.h"
#include "swftypes.h"
#include <vector>
#include <list>

extern std::list < RenderTag* > dictionary;

Tag* TagFactory::readTag()
{
	RECORDHEADER h;
	f >> h;
	switch(h>>6)
	{
		case 0:
			std::cout << "position " << f.tellg() << std::endl;
			return new EndTag(h,f);
		case 1:
			std::cout << "position " << f.tellg() << std::endl;
			return new ShowFrameTag(h,f);
		case 2:
			std::cout << "position " << f.tellg() << std::endl;
			return new DefineShapeTag(h,f);
	//	case 4:
	//		return new PlaceObjectTag(h,f);
		case 9:
			std::cout << "position " << f.tellg() << std::endl;
			return new SetBackgroundColorTag(h,f);
		case 11:
			std::cout << "position " << f.tellg() << std::endl;
			return new DefineTextTag(h,f);
		case 26:
			std::cout << "position " << f.tellg() << std::endl;
			return new PlaceObject2Tag(h,f);
		case 39:
			std::cout << "position " << f.tellg() << std::endl;
			return new DefineSpriteTag(h,f);
		case 45:
			std::cout << "position " << f.tellg() << std::endl;
			return new SoundStreamHead2Tag(h,f);
		case 48:
			std::cout << "position " << f.tellg() << std::endl;
			return new DefineFont2Tag(h,f);
		default:
			std::cout << "position " << f.tellg() << std::endl;
			std::cout << (h>>6) << std::endl;
			throw "unsupported tag";
			break;
	}
}


SetBackgroundColorTag::SetBackgroundColorTag(RECORDHEADER h, std::istream& in):ControlTag(h,in)
{
	in >> BackgroundColor;
	std::cout << BackgroundColor << std::endl;
}

DefineSpriteTag::DefineSpriteTag(RECORDHEADER h, std::istream& in):RenderTag(h,in)
{
	std::cout << "DefineSprite" << std::endl;
	in >> SpriteID >> FrameCount;
	if(FrameCount!=1)
		throw "unsopported long sprites";
	TagFactory factory(in);
	Tag* tag;
	do
	{
		tag=factory.readTag();	
		std::cout << "\tsprite tag type "<< tag->getType() <<std::endl;
		if(tag->getType()==DISPLAY_LIST_TAG)
			displayList.push_back(dynamic_cast<DisplayListTag*>(tag));
	}
	while(tag->getType()!=END_TAG);
}

DefineFont2Tag::DefineFont2Tag(RECORDHEADER h, std::istream& in):RenderTag(h,in)
{
	std::cout << "DefineFont2" << std::endl;
	in >> FontID;
	BitStream bs(in);
	FontFlagsHasLayout = UB(1,bs);
	FontFlagsShiftJIS = UB(1,bs);
	FontFlagsSmallText = UB(1,bs);
	FontFlagsANSI = UB(1,bs);
	FontFlagsWideOffsets = UB(1,bs);
	FontFlagsWideCodes = UB(1,bs);
	FontFlagsItalic = UB(1,bs);
	FontFlagsBold = UB(1,bs);
	in >> LanguageCode >> FontNameLen;
	for(int i=0;i<FontNameLen;i++)
	{
		UI8 t;
		in >> t;
		FontName.push_back(t);
	}
	in >> NumGlyphs;
	UI16 t;
	if(FontFlagsWideOffsets)
	{
		throw "help7";
	}
	else
	{
		for(int i=0;i<NumGlyphs;i++)
		{
			in >> t;
			OffsetTable.push_back(t);
		}
		in >> t;
		CodeTableOffset=t;
	}
	for(int i=0;i<NumGlyphs;i++)
	{
		std::cout << "font read " << i << std::endl;
		SHAPE t;
		in >> t;
		GlyphShapeTable.push_back(t);
	}
	if(FontFlagsWideCodes)
	{
		throw "help8";
	}
	else
	{
		for(int i=0;i<NumGlyphs;i++)
		{
			UI8 t;
			in >> t;
			CodeTable.push_back(t);
		}
	}
	if(FontFlagsHasLayout)
	{
		in >> FontAscent >> FontDescent >> FontLeading;

		for(int i=0;i<NumGlyphs;i++)
		{
			SI16 t;
			in >> t;
			FontAdvanceTable.push_back(t);
		}
		for(int i=0;i<NumGlyphs;i++)
		{
			RECT t;
			in >> t;
			FontBoundsTable.push_back(t);
		}
		in >> KerningCount;
	}
	//Stub implement kerninfo merda merda
	in.ignore(KerningCount*4);
}

DefineTextTag::DefineTextTag(RECORDHEADER h, std::istream& in):RenderTag(h,in)
{
	std::cout << "DefineText" << std::endl;
	in >> CharacterId >> TextBounds >> TextMatrix >> GlyphBits >> AdvanceBits;
	while(1)
	{
		TEXTRECORD t(this);
		in >> t;
		if(t.TextRecordType+t.StyleFlagsHasFont+t.StyleFlagsHasColor+t.StyleFlagsHasYOffset+t.StyleFlagsHasXOffset==0)
			break;
		TextRecords.push_back(t);
	}
}

SoundStreamHead2Tag::SoundStreamHead2Tag(RECORDHEADER h, std::istream& in):Tag(h,in)
{
	std::cout << "STUB SoundStreamHead2" << std::endl;
	if((h&0x3f)==0x3f)
		in.ignore(Length);
	else
		in.ignore(h&0x3f);
}

DefineShapeTag::DefineShapeTag(RECORDHEADER h, std::istream& in):RenderTag(h,in)
{
	in >> ShapeId >> ShapeBounds >> Shapes;
}

class Vector2
{
public:
	int x,y;
	Vector2(int a, int b):x(a),y(b){}
	bool operator==(const Vector2& v){return v.x==x && v.y==y;}
};

class Path
{
public:
	std::vector< Vector2 > points;
	bool closed;
	Path():closed(false){};
};

void DefineSpriteTag::Render()
{
	std::cout << "Render Sprite" << std::endl;
	std::list < DisplayListTag* >::iterator it=displayList.begin();
	for(it;it!=displayList.end();it++)
	{
		GLObject* cur=(*it)->Render();
		glCallList(cur->list);
	}
}

class GraphicStatus
{
public:
	bool validFill;
	bool validStroke;

	GraphicStatus():validFill(false),validStroke(false){}
};

void TessellateShaperecordList(SHAPERECORD* cur, std::vector< Path >& paths, GraphicStatus& status);

void DefineShapeTag::Render()
{
	std::cout << "Render Shape" << std::endl;
	int startX=0,startY=0;
	SHAPERECORD* cur=&(Shapes.ShapeRecords);
	std::vector< Path > paths;
	GraphicStatus status;
	TessellateShaperecordList(cur,paths,status);
	std::vector < Path >::iterator it=paths.begin();
	for(it;it!=paths.end();it++)
	{
		std::vector < Vector2 >::iterator it2=(*it).points.begin();
		if(status.validFill && (*it).closed)
		{
			glBegin(GL_TRIANGLE_FAN);
			for(it2;it2!=(*it).points.end();it2++)
				glVertex2i((*it2).x,(*it2).y);
			glEnd();
		}
		it2=(*it).points.begin();
		if(status.validStroke)
		{
			if((*it).closed)
				glBegin(GL_LINE_LOOP);
			else
				glBegin(GL_LINE_STRIP);
			for(it2;it2!=(*it).points.end();it2++)
				glVertex2i((*it2).x,(*it2).y);
			glEnd();
		}
	}
}

void TessellateShaperecordList(SHAPERECORD* cur, std::vector< Path >& paths, GraphicStatus& status)
{
	int start2X=0,start2Y=0;
//#define startX (start2X*0.351562)
//#define startY (start2Y*0.351562)
	#define startX start2X
	#define startY start2Y
	while(cur)
	{
		if(cur->TypeFlag)
		{
			if(cur->StraightFlag)
			{
				start2X+=cur->DeltaX;
				start2Y+=cur->DeltaY;
				if(Vector2(startX,startY)==paths.back().points.front())
					paths.back().closed=true;
				else
					paths.back().points.push_back(Vector2(startX,startY));
			}
			else
			{
				start2X+=cur->ControlDeltaX;
				start2Y+=cur->ControlDeltaY;
				if(Vector2(startX,startY)==paths.back().points.front())
					paths.back().closed=true;
				else
					paths.back().points.push_back(Vector2(startX,startY));

				start2X+=cur->AnchorDeltaX;
				start2Y+=cur->AnchorDeltaY;
				if(Vector2(startX,startY)==paths.back().points.front())
					paths.back().closed=true;
				else
					paths.back().points.push_back(Vector2(startX,startY));
			}
		}
		else
		{
			if(cur->StateMoveTo)
			{
				paths.push_back(Path());
				start2X=cur->MoveDeltaX;
				start2Y=cur->MoveDeltaY;
				paths.back().points.push_back(Vector2(startX,startY));
			}
			if(cur->StateLineStyle)
			{
/*				glLineWidth(Shapes.LineStyles.LineStyles[cur->LineStyle].Width);
				glColor3ub(Shapes.LineStyles.LineStyles[cur->LineStyle].Color.Red,
						Shapes.LineStyles.LineStyles[cur->LineStyle].Color.Green,
						Shapes.LineStyles.LineStyles[cur->LineStyle].Color.Blue);
*/
				if(cur->LineStyle)
					status.validStroke=true;
				else
					status.validStroke=false;
			}
			if(cur->StateFillStyle1)
			{
		/*		glColor4ub(Shapes.FillStyles.FillStyles[cur->FillStyle1].Color.Red,
						Shapes.FillStyles.FillStyles[cur->FillStyle1].Color.Green,
						Shapes.FillStyles.FillStyles[cur->FillStyle1].Color.Blue);*/
			}
			if(cur->StateFillStyle0)
			{
				if(cur->FillStyle0!=1)
					throw "fill style0";
				status.validStroke=true;
			}
		}
		cur=cur->next;
	}
}


void DefineTextTag::Render()
{
	std::cout << "Render Text" << std::endl;
	std::cout << TextMatrix << std::endl;
	glColor3f(0,0,0);
	std::vector < TEXTRECORD >::iterator it= TextRecords.begin();
	int x=0,y=0;//1024;
	std::vector < GLYPHENTRY >::iterator it2;
	DefineFont2Tag* font=NULL;
	//glScalef(0.351562,0.351562,1);
	for(it;it!=TextRecords.end();it++)
	{
		std::cout << "Text height " << it->TextHeight << std::endl;
		if(it->StyleFlagsHasFont)
		{
			std::list< RenderTag*>::iterator it3 = dictionary.begin();
			for(it3;it3!=dictionary.end();it3++)
			{
				if((*it3)->getId()==it->FontID)
				{
					std::cout << "Looking for Font " << it->FontID << std::endl;
					break;
				}
			}
			font=dynamic_cast<DefineFont2Tag*>(*it3);
		}
		it2 = it->GlyphEntries.begin();
		int x2=x,y2=y;
		int done=0;
		x2+=(*it).XOffset;
		y2+=(*it).YOffset;
		for(it2;it2!=(it->GlyphEntries.end());it2++)
		{
			SHAPE& shape=font->GlyphShapeTable[it2->GlyphIndex];
			SHAPERECORD* cur=&(shape.ShapeRecords);
			std::vector< Path > paths;
			GraphicStatus status;
			TessellateShaperecordList(cur,paths,status);
			std::vector < Path >::iterator it=paths.begin();
			for(it;it!=paths.end();it++)
			{
				glPushMatrix();
				glTranslatef(x2,y2,0);
				glScalef(0.351562,0.351562,1);
				std::vector < Vector2 >::iterator it2=(*it).points.begin();
				if(status.validFill && (*it).closed)
				{
					glBegin(GL_TRIANGLE_FAN);
					for(it2;it2!=(*it).points.end();it2++)
						glVertex2i((*it2).x,(*it2).y);
					glEnd();
				}
				it2=(*it).points.begin();
				if(status.validStroke)
				{
					if((*it).closed)
						glBegin(GL_LINE_LOOP);
					else
						glBegin(GL_LINE_STRIP);
					for(it2;it2!=(*it).points.end();it2++)
						glVertex2i((*it2).x,(*it2).y);
					glEnd();
				}
				glPopMatrix();
			}
			std::cout << "Character " << it2->GlyphIndex << std::endl;
			x2+=it2->GlyphAdvance;
		}
	}
}

ShowFrameTag::ShowFrameTag(RECORDHEADER h, std::istream& in):Tag(h,in)
{
	std::cout <<"ShowFrame" << std::endl;
}

PlaceObject2Tag::PlaceObject2Tag(RECORDHEADER h, std::istream& in):DisplayListTag(h,in)
{
	std::cout << "PlaceObject2" << std::endl;
	BitStream bs(in);
	PlaceFlagHasClipAction=UB(1,bs);
	PlaceFlagHasClipDepth=UB(1,bs);
	PlaceFlagHasName=UB(1,bs);
	PlaceFlagHasRatio=UB(1,bs);
	PlaceFlagHasColorTransform=UB(1,bs);
	PlaceFlagHasMatrix=UB(1,bs);
	PlaceFlagHasCharacter=UB(1,bs);
	PlaceFlagMove=UB(1,bs);
	in >> Depth;
	if(PlaceFlagHasClipAction+PlaceFlagHasName+PlaceFlagHasRatio+PlaceFlagMove)
	{
		throw "unsupported has";
	}
	if(PlaceFlagHasCharacter)
	{
		in >> CharacterId;
		std::cout << "new character ID " << CharacterId << std::endl;
	}
	if(PlaceFlagHasMatrix)
		in >> Matrix;
	if(PlaceFlagHasClipDepth)
		in >> ClipDepth;
	if(PlaceFlagHasColorTransform)
		in >> ColorTransform;
}

GLObject* PlaceObject2Tag::Render()
{
	std::cout << "Render Place object 2 ChaID " << CharacterId <<  std::endl;
	std::cout << Matrix << std::endl;
	if(!PlaceFlagHasCharacter)
		throw "modify not supported";
	std::list< RenderTag* >::iterator it=dictionary.begin();
	for(it;it!=dictionary.end();it++)
	{
		std::cout << "ID " << dynamic_cast<RenderTag*>(*it)->getId() << std::endl;
		if(dynamic_cast<RenderTag*>(*it)->getId()==CharacterId)
			break;
	}
	if(it==dictionary.end())
	{
		throw "Object does not exist";
		return new GLObject;
	}
	if((*it)->getType()!=RENDER_TAG)
		throw "cazzo renderi";
	GLObject* ret=new GLObject();
	
	float matrix[16];
	Matrix.get4DMatrix(matrix);
	ret->start();
	glMultMatrixf(matrix);
	{
		dynamic_cast<RenderTag*>(*it)->Render();
	}
	ret->end();
	return ret;
}

void SetBackgroundColorTag::execute()
{
	std::cout << "execute SetBG" <<  std::endl;
	glClearColor(BackgroundColor.Red/255.0F,BackgroundColor.Green/255.0F,BackgroundColor.Blue/255.0F,0);
}
