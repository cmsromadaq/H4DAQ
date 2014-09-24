#include "interface/Utility.hpp"
#include <sstream>

string Utility::AsciiData(void *data,int N,int M)
{
string mex="";
char buf[4];
for(int i=0;i<N ;i++)
	{
	sprintf(buf,"%02X",((unsigned char*)data)[i] );
	mex += buf;
	if ((i%M)==M-1) mex += "\n";
	else mex += " ";
	}
if( (N-1)%M != M-1 ) mex += "\n";
return mex;
}

string Utility::AsciiDataReadable(void *data,int N)
{
string mex="";
char buf[4];
for(int i=0;i<N ;i++)
	{
	char c=((unsigned char*)data)[i];
	if (  ('a' <= c && c<='z' ) ||
	      ('A' <= c && c<='Z' ) ||
	      ('0' <= c && c<='9' ) )
		{ mex += c; continue; }
	sprintf(buf,"%02X",((unsigned char*)data)[i] );
	mex += " '" ;
	mex += buf;
	mex += "' ";
	}
return mex;
}

string Utility::AsciiData(void *data,void *data2,int N,int M)
{
string mex="";
string mex1=AsciiData(data,N,M);
string mex2=AsciiData(data2,N,M);

// put together lines
while( !mex1.empty() && !mex2.empty() )
	{
	size_t pos1=mex1.find("\n");
	size_t pos2=mex2.find("\n");
	mex += mex1.substr(0,pos1) + "     " + mex2.substr(0,pos2) + "\n";
	mex1.erase(0,pos1+1);
	mex2.erase(0,pos2+1);
	}
return mex;
}

string Utility::ConvertToString(int N){
	std::ostringstream ostr; //output string stream
	ostr << N; //use the string stream just like cout,
	std::string theNumberString = ostr.str(); //the str() function of the stream 
	return theNumberString;
}
