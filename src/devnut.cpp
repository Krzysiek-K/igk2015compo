
#include "devnut.h"


SqVM vm;




void xsq_print( const char *msg )
{
	printf("%s",msg);
	OutputDebugString(msg);
}

void xsq_error( const char *err )
{
	printf("ERROR: %s",err);
	OutputDebugString("ERROR: ");
	OutputDebugString(err);
}

bool xsq_getfile( const char *path, std::vector<unsigned char> &data )
{
	return NFS.GetFileBytes(path,data);
}

unsigned long long xsq_getfiletime( const char *path )
{
	return GetFileTime(path);
}



void NutInit()
{
	vm.Init(2048);
	vm.AddFile("util.nut");
	vm.AddFile("vector.nut");
}
