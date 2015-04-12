
#include "base.h"

using namespace std;



namespace base
{

int __convert_cpp_no_exports_warning_disable;


/*
#define EPSILON 10E-8
static int _skpwhts(const char *&str) { 
	const char *begin = str; 

	while(*str==' ' || *str=='\t' || *str=='\r' || *str=='\n') str++;  
	
	return int(str-begin);
}

static int _atoi(const char *str, int &out)
{
	const char *p = str;
	int val=0;
	while(*p>='0' && *p<='9')
	{
		val*=10; val+=*p-'0'; 
		p++;
	}
	out=val; return int(p-str);
}

static int _htoi(const char *str, int &out)
{
	const char *p = str;
	int val=0;
	while((*p>='0' && *p<='9') || (*p>='a' && *p<='f') || (*p>='A' && *p<='F'))
	{
		int v=0;
		if	   (*p>='A' && *p<='F')	 v=10+*p-'A';
		else if(*p>='a' && *p<='f')  v=10+*p-'a';
		else						 v=   *p-'0';
		
		val<<=4; val+=v; 
		p++;
	}
	out=val; return int(p-str);
}


#define CHK_END(ch) if(ch=='\0') return 0;

int	Conversions::StringToInt(const char *s,int &out)
{
	const char *p = s;
	int sign = 1;
	_skpwhts(p);

	CHK_END(*p)

	if		(*p == '+')			  ++p;
	else if (*p == '-') {sign=-1; ++p;}
	
	CHK_END(*p)

//	_skpwhts(p);

//	CHK_END(*p)
	

	int ret = 0;
	if(*p == '0' )
	{
		p++;
		if (*p != 'x' && *p != 'X') {out = 0; return p-s;}
		p++;

		ret=_htoi(p, out);
		if(ret == 0) return (p-s-1);
				
	} else
	{
		ret=_atoi(p, out);
		if(ret == 0) return 0;
	}
	
	out*=sign;
	return int(p-s+ret);
}

int	Conversions::StringToFloat(const char *s,float &out)
{
	int n=0;
	int val1,val2, dig=0, sign=1;

	const char *p = s;

	_skpwhts(p);

	CHK_END(*p)

	if		(*p == '+')			  ++p;
	else if (*p == '-') {sign=-1; ++p;}
	
	CHK_END(*p)

	_skpwhts(p);

	CHK_END(*p)
	
	n=_atoi(p,val1);
	if(n == 0) val1=0;
	p+=n;

	if(*p=='.')
	{
		p++;
		if(p=='\0')
		{
			out=val1*(float)sign;
			return int(p-s);
		}
		
		dig=_atoi(p, val2);
		if(dig!=0)
		{
			double f=1;
			for(int i=0; i<dig; i++) f*=0.1f;
			
			out=float(double(val1)+double(val2)*f);

			p+=dig;
		}

		float eval=10, f=1;
		int exp;
		if(*p=='E' || *p=='e')
		{
			p++;
			if		(*p == '+')	           ++p;
			else if (*p == '-') {eval=0.1f; ++p;}

			dig=_atoi(p, exp);

			if(dig!=0)
			{
				p+=dig;
				while(--exp >=0) f*=eval;
				out*=f;
			} else return int(p-s-1);
		}
	}
	else
		out = val1;
	out*=sign;
	return int(p-s);	
}

int	Conversions::StringToIntArray(const char *s,int *arr)
{
	const char *p = s;
	
	int val, n, num=0;
	do {
		n = StringToInt(p, val);
		if(n == 0) break;

		*arr++ = val;
		
		p+=n;
		num++;
		if(*p==',') p++; else break;
	}
	while(1);
	return num;
}

int	Conversions::StringToFloatArray(const char *s,float *arr)
{
	const char *p = s;
	
	int n, num=0;
	float val;
	do {
		n = StringToFloat(p, val);
		if(n == 0) break;

		*arr++ = val;
		
		p+=n;
		num++;
		if(*p==',') p++; else break;
	}
	while(1);
	return num;
}

int	Conversions::StringGetArraySize(const char *s)
{
	const char *p = s;

	_skpwhts(p);
	int num=0;
	while(*p!= '\0')
	{
		while(*p != '\0' && *p!=' ' && *p!='\n' && *p!='\r' && *p!='\t')
			p++;
		
		_skpwhts(p);

		num++;
		if(*p=='\0') return num;
		p++;
	
		_skpwhts(p);
	}
	return num;
}

void Conversions::IntToString(int value,string &str)
{
	//TODO: finish it
	bool positive = value > 0;
	if(value == 0) {str="0"; return;}
	if(!positive)  {value=-value; str='-';} else str="";

	while(value > 0) {str+=(char)(value%10+'0'); value/=10;};

	int i, j;
	for(j=(int)str.length()-1,i=positive?0:1; i < j; i++, j--)
	{
		char t = str[j];
		str[j] = str[i];
		str[i] = t;
	}
}

void Conversions::FloatToString(float value,string &str)
{
	//cutprec(1.1111100001);
	int exp=0;
	double _value = value;
	bool positive = _value > 0;
	if(!positive)  {_value=-_value; str='-';} else str="";
	
	if(_value==0.0f) {str="0.0"; return;}

	if(_value > EXP_UPPER_LIMIT) 
	{
		while(_value>=1.f) {_value/=10;exp++;}
		_value*=10;
		exp--;
	} else
	if(_value < EXP_LOWER_LIMIT) 
	{
		while(_value<1.f) {_value*=10;exp--;}
	}

	double frac, in;
	in = (int) _value;
	frac = _value-in;

	double temp=(float)frac;
	int nd=0;
	while(temp-int(temp) > EPSILON && nd<7) {temp*=10;nd++;}
	string str1, str2;

	IntToString((int)in, str1);
	IntToString((int)temp, str2);
	nd-= (int)str2.length();

	//remove trailing zeros
	int i;
	for(i=(int)str2.length()-1; i>=0 && str2[i]=='0'; i--);
	
	str2 = str2.substr(0, i+1);

	if(str2.size() >= 8) str2 = str2.substr(0, 8);

	str = str1+".";
	while(--nd >= 0) str+="0";

	str += str2;

	string strE;
	if(exp!=0) {IntToString(exp, strE);strE = "E"+strE;}

	str += strE;
	if(!positive) str = "-"+str;
	return;
}

int	Conversions::IntArrayToString(int *data,int count,string &str)
{
	str="";
	
	string temp;
	for(int i=0; i<count; i++) {IntToString(*data, temp); str+=temp+","; data++;}

	return 0;
}

int	Conversions::FloatArrayToString(float *data,int count,string &str)
{
	str="";
	
	string temp;
	for(int i=0; i<count; i++) {FloatToString(*data, temp);str+=temp+",";data++;}
	
	return 0;

}
*/

}
