#include "swftypes.h"

RECT::RECT()
{
}

/*RECT::RECT(FILE* in)
{
	BitStream s(in);
	NBits=UB(5,s);
	Xmin=SB(NBits,s);
	Xmax=SB(NBits,s);
	Ymin=SB(NBits,s);
	Ymax=SB(NBits,s);
}*/

std::ostream& operator<<(const std::ostream& s, const RECT& r)
{
	std::cout << '{' << (int)r.Xmin << ',' << r.Xmax << ',' << r.Ymin << ',' << r.Ymax << '}';
}

std::ostream& operator<<(const std::ostream& s, const RGB& r)
{
	std::cout << "RGB <" << (int)r.Red << ',' << (int)r.Green << ',' << (int)r.Blue << '>';
}

std::istream& operator>>(std::istream& stream, RECT& v)
{
	BitStream s(stream);
	v.NBits=UB(5,s);
	v.Xmin=SB(v.NBits,s);
	v.Xmax=SB(v.NBits,s);
	v.Ymin=SB(v.NBits,s);
	v.Ymax=SB(v.NBits,s);
	return stream;
}

std::istream& operator>>(std::istream& s, RGB& v)
{
	s >> v.Red >> v.Green >> v.Blue;
	return s;
}

std::istream& operator>>(std::istream& s, LINESTYLEARRAY& v)
{
	s >> v.LineStyleCount;
	if(v.LineStyleCount==0xff)
		throw "Line array extended not supported\n";
	std::cout << "Reading " << (int)v.LineStyleCount << " Line styles" << std::endl;
	v.LineStyles=new LINESTYLE[v.LineStyleCount];
	for(int i=0;i<v.LineStyleCount;i++)
	{
		s >> v.LineStyles[i];
	}
	return s;
}

std::istream& operator>>(std::istream& s, FILLSTYLEARRAY& v)
{
	s >> v.FillStyleCount;
	if(v.FillStyleCount==0xff)
		throw "Fill array extended not supported\n";
	std::cout << "Reading " << (int)v.FillStyleCount << " Fill styles" << std::endl;
	v.FillStyles=new FILLSTYLE[v.FillStyleCount];
	for(int i=0;i<v.FillStyleCount;i++)
	{
		s >> v.FillStyles[i];
	}
	return s;
}

std::istream& operator>>(std::istream& s, SHAPEWITHSTYLE& v)
{
	s >> v.FillStyles >> v.LineStyles;
	BitStream bs(s);
	v.NumFillBits=UB(4,bs);
	v.NumLineBits=UB(4,bs);
	v.ShapeRecords=SHAPERECORD(&v,bs);
	if(v.ShapeRecords.StateNewStyles+v.ShapeRecords.StateLineStyle+v.ShapeRecords.StateFillStyle1+v.ShapeRecords.StateFillStyle0+v.ShapeRecords.StateMoveTo)
	{
		SHAPERECORD* cur=&(v.ShapeRecords);
		while(1)
		{
			cur->next=new SHAPERECORD(&v,bs);
			cur=cur->next;
			if(cur->TypeFlag+cur->StateNewStyles+cur->StateLineStyle+cur->StateFillStyle1+cur->StateFillStyle0+cur->StateMoveTo==0)
				break;
		}
	}

	//s >> v.ShapeRecords;
}

std::istream& operator>>(std::istream& s, LINESTYLE& v)
{
	s >> v.Width >> v.Color;
	std::cout << "Line " << v.Width/20 << v.Color;
	return s;
}

std::istream& operator>>(std::istream& s, FILLSTYLE& v)
{
	s >> v.FillStyleType;
	if(v.FillStyleType!=0)
		throw "unsupported fill type";
	s >> v.Color;
	std::cout << "Fill color " << v.Color << std::endl;
	return s;
}

SHAPERECORD::SHAPERECORD(SHAPEWITHSTYLE* p,BitStream& bs):parent(p),next(0)
{
	std::cout << "start shaperecord" << std::endl;
	TypeFlag = UB(1,bs);
	if(TypeFlag)
	{
		std::cout << "edge shaperecord" << std::endl;
		StraightFlag=UB(1,bs);
		NumBits=UB(4,bs);
		if(StraightFlag)
		{
			std::cout << "\tstraight line bits " << NumBits+2<< std::endl;

			GeneralLineFlag=UB(1,bs);
			if(GeneralLineFlag)
				throw "general line";
			else
				VertLineFlag=UB(1,bs);

			if(GeneralLineFlag || !VertLineFlag)
			{
				DeltaX=SB(NumBits+2,bs);
				std::cout << "DeltaX " << DeltaX << std::endl;
			}
			if(GeneralLineFlag || VertLineFlag)
			{
				DeltaY=SB(NumBits+2,bs);
				std::cout << "DeltaY " << DeltaY << std::endl;
			}
		}
		else
		{
			std::cout << "\tcurve line bits " << NumBits+2 << std::endl;
			
			ControlDeltaX=SB(NumBits+2,bs);
			ControlDeltaY=SB(NumBits+2,bs);
			AnchorDeltaX=SB(NumBits+2,bs);
			AnchorDeltaY=SB(NumBits+2,bs);
			
		}
	}
	else
	{
		std::cout << "non edge shaperecord" << std::endl;
		StateNewStyles = UB(1,bs);
		StateLineStyle = UB(1,bs);
		StateFillStyle1 = UB(1,bs);
		StateFillStyle0 = UB(1,bs);
		StateMoveTo = UB(1,bs);
		if(StateNewStyles || StateLineStyle || StateFillStyle0 )
		{
			throw "unsupported States in shaperecord";
		}
		if(StateMoveTo)
		{
			std::cout << "move to" << std::endl;
			MoveBits = UB(5,bs);
			std::cout <<"\tbits " << MoveBits << std::endl;
			MoveDeltaX = UB(MoveBits,bs);
			MoveDeltaY = UB(MoveBits,bs);
		}
		if(StateFillStyle1)
		{
			std::cout << "fill style 1 bits " << parent->NumFillBits << std::endl;
			FillStyle1=UB(parent->NumFillBits,bs);
		}
	}
}

/*std::istream& operator>>(std::istream& stream, SHAPERECORD& v)
{
	std::cout << "start shaperecord" << std::endl;
	BitStream bs(stream);
	v.TypeFlag = UB(1,bs);
	if(v.TypeFlag)
	{
		std::cout << "edge shaperecord" << std::endl;
		v.StraightFlag=UB(1,bs);
		v.NumBits=UB(4,bs);
		if(v.StraightFlag)
		{
			std::cout << "\tstraight line bits " << v.NumBits+2<< std::endl;

			v.GeneralLineFlag=UB(1,bs);
			if(v.GeneralLineFlag)
				throw "general line";
			else
				v.VertLineFlag=UB(1,bs);

			if(v.GeneralLineFlag || !v.VertLineFlag)
				throw "deltax";
			if(v.GeneralLineFlag || v.VertLineFlag)
			{
				v.DeltaY=SB(v.NumBits+2,bs);
				std::cout << "DeltaY " << v.DeltaY << std::endl;
			}
		}
		else
		{
			std::cout << "\tcurve line bits " << v.NumBits+2 << std::endl;
			
			v.ControlDeltaX=SB(v.NumBits+2,bs);
			v.ControlDeltaY=SB(v.NumBits+2,bs);
			v.AnchorDeltaX=SB(v.NumBits+2,bs);
			v.AnchorDeltaY=SB(v.NumBits+2,bs);
			
		}

		std::cout << "\t avanzano " << (int)bs.pos << std::endl;
		v.next=new SHAPERECORD(v.parent);
		stream >> *v.next;
	}
	else
	{
		std::cout << "non edge shaperecord" << std::endl;
		v.StateNewStyles = UB(1,bs);
		v.StateLineStyle = UB(1,bs);
		v.StateFillStyle1 = UB(1,bs);
		v.StateFillStyle0 = UB(1,bs);
		v.StateMoveTo = UB(1,bs);
		if(v.StateNewStyles || v.StateLineStyle || v.StateFillStyle0 )
		{
			throw "unsupported States in shaperecord";
		}
		if(v.StateMoveTo)
		{
			std::cout << "move to" << std::endl;
			v.MoveBits = UB(5,bs);
			std::cout <<"\tbits " << v.MoveBits << std::endl;
			v.MoveDeltaX = UB(v.MoveBits,bs);
			v.MoveDeltaY = UB(v.MoveBits,bs);
		}
		if(v.StateFillStyle1)
		{
			std::cout << "fill style 1 bits " << v.parent->NumFillBits << std::endl;
			v.FillStyle1=UB(v.parent->NumFillBits,bs);
		}

		if(v.StateNewStyles+v.StateLineStyle+v.StateFillStyle1+v.StateFillStyle0+v.StateMoveTo)
		{
			v.next=new SHAPERECORD(v.parent);
			stream >> *v.next;
		}
		else
			std::cout <<  "end shape" << std::endl;
	}

	return stream;
}*/
