struct Numeric_Edge
{
	int a,b;
	int len;
	Numeric_Edge(int x, int y,int num)
	{
		if(((x-y+num)%num)<((y-x+num)%num))
		{
			a=y;
			b=x;
			len=((x-y+num)%num);
		}
		else
		{
			a=x;
			b=y;
			len=((y-x+num)%num);
		}
	}
	bool operator<(const Numeric_Edge& e) const
	{
		return len<e.len;
	}
};

