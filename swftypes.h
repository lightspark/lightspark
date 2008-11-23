#include <stdint.h>
#include <stdio.h>
#include <iostream>
typedef uint8_t UI8;
typedef uint16_t UI16;
typedef uint32_t UI32;

inline int min(int a,int b)
{
	return (a<b)?a:b;
}

class BitStream
{
private:
	FILE* f;
	unsigned char buffer;
	unsigned char pos;
public:
	BitStream(FILE* in):f(in),pos(0){};
	unsigned int readBits(unsigned int num)
	{
		unsigned int ret=0;
		while(num)
		{
			if(!pos)
			{
				pos=8;
				fread(&buffer,1,1,f);
			}
			ret|=(buffer>>(pos-1))&1;
			ret<<=1;
			pos--;
			num--;
		}
		return ret;
	}
};

class UB
{
	uint32_t buf;
	int size;
public:
	UB() { buf=NULL; }
	UB(int s,BitStream& stream):size(s)
	{
/*		if(s%8)
			buf=new uint8_t[s/8+1];
		else
			buf=new uint8_t[s/8];
		int i=0;
		while(!s)
		{
			buf[i]=stream.readBits(min(s,8));
			s-=min(s,8);
			i++;
		}*/
		if(s>32)
			throw "UB too big\n";
		buf=stream.readBits(s);
	}
	operator int()
	{
		return buf;
	}
};

class SB
{
	int32_t buf;
	int size;
public:
	SB() { buf=NULL; }
	SB(int s,BitStream& stream):size(s)
	{
		if(s>32)
			throw "SB too big\n";
		buf=stream.readBits(s);
		if(buf>>(s-1)&1)
		{
			for(int i=31;i>=s;i--)
				buf|=(1<<i);
		}
	}
	operator int() const
	{
		return buf;
	}
};

class RECT
{
	friend std::ostream& operator<<(const std::ostream& s, const RECT& r);
private:
	UB NBits;
	SB Xmin;
	SB Xmax;
	SB Ymin;
	SB Ymax;
public:
	RECT();
	RECT(FILE* in);
};

std::ostream& operator<<(const std::ostream& s, const RECT& r);
