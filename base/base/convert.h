
#ifndef _CONVERT_H
#define _CONVERT_H



namespace base
{

/*
const float EXP_UPPER_LIMIT = 1E6f;
const float EXP_LOWER_LIMIT = 1E-6f;

class Conversions {
public:
//	static const float EXP_UPPER_LIMIT = 1E6;
//	static const float EXP_LOWER_LIMIT = 1E-6;

// conversion rules:
// - functions return count of characters used (including leading whitespace)
// - if a function failed to convert a number it should return zero and keep return variable untouched
// - all leading whitespace characters (space, \t, \n, \r) are skipped
// - both decimal and hexadecimal (preceded with 0x) integers should be accepted
// - floats written in decimal (123.456) and scientific (123.456e+20) should be accepted
// - every number can be preceded with optional '+' (positive, default) or '-' (negative)
// - int->string conversion should use decimal format
// - float->string conversion should use standard format with enough precision
// - for very large and very small float values scientific format should be used automatically
// - ->string conversions should clear output string variable first
// - array is a set of values separated by optional comas
// - if value in an array is omited, zero should be used by default
// - examples
// - ""			-> { }			(zero elements)
// - "123"		-> { 123 }
// - "1 2 3"	-> { 1, 2, 3 }
// - "1, 2 3"	-> { 1, 2, 3 }
// - "1,,2,3"	-> { 1, 0, 2, 3 }


	static int	StringToInt(const char *s,int &out);
	static int	StringToFloat(const char *s,float &out);

	static int	StringToIntArray(const char *s,int *arr);
	static int	StringToFloatArray(const char *s,float *arr);

	static int	StringGetArraySize(const char *s);


	static void IntToString(int value,std::string &str);
	static void FloatToString(float value,std::string &str);

	static int	IntArrayToString	(int *data,int count,std::string &str);
	static int	FloatArrayToString	(float *data,int count,std::string &str);

};
*/

}




#endif
