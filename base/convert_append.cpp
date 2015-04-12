#include "base.h"


using namespace std;


namespace base
{




void AppendFloat(string &str,float value)
{
    int sign = (*(DWORD*)&value >> 31) & 1;
    int exp  = (*(DWORD*)&value >> 23) & 0xFF;
    int vv   = (*(DWORD*)&value ) & 0x7FFFFF;

    if(exp==0xFF)
    {
        if(sign) str.push_back('-');
        if(!vv) str += "1.#Inf";
        else str += "1.#QNAN";
        return;
    }

    if(exp<0x7F || exp>0x7F+20)
        str += format("%.8e",value);
    else
        str += format("%.8f",value);
}

void AppendString(std::string &str,const char *v,const char *escape)
{
    DWORD esc[8] = {0xFFFFFFFF, (1<<(' '-32)) | (1<<('"'-32)), (1<<('\\'-64))};
    while(*escape)
    {
        esc[byte(*escape)/32] |= (1 << (byte(*escape)%32));
        escape++;
    }

    const char *s = v;
    int flag = 0;
    while(*s)
    {
        flag |= ( esc[byte(*s)>>5] >> (byte(*s)&31) );
        s++;
    }

    if(*v && !(flag&1))
    {
        str += v;
        return;
    }

    str.push_back('"');
    s = v;
    while(*s)
    {
        if( (*s>=0 && *s<32) || *s=='"' || *s=='\\')
        {
            str.push_back('\\');
            if(byte(*s)>=32) str.push_back(*s);
            else if(*s=='\n') str.push_back('n');
            else if(*s=='\r') str.push_back('r');
            else if(*s=='\t') str.push_back('t');
            else if(*s=='\b') str.push_back('b');
            else
            {
                str.push_back('0');
                str.push_back(*s/8+'0');
                str.push_back(*s%8+'0');
            }
        }
        else
            str.push_back(*s);
        s++;
    }
    str.push_back('"');
}

void AppendHexBuffer(std::string &str,const void *v,int size)
{
    const byte *ptr = *(const byte **)&v;
    while(size-->0)
    {
        str.push_back("0123456789ABCDEF"[*ptr>>4]);
        str.push_back("0123456789ABCDEF"[*ptr&15]);
        ptr++;
    }
}




}
