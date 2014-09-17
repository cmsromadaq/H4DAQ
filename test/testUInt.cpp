#include <cstdio>


int main(){
	unsigned char c;
	unsigned short s;
	unsigned int i;
	unsigned long int li;
	unsigned long long int lli;
	float f;
	double d;
	long double ld;
	
	printf("**** SIZE OF ******\n");
	printf("char %lu == 1\n",sizeof(c));
	printf("short %lu\n",sizeof(s));
	printf("int %lu == 4\n",sizeof(i));
	printf("long %lu\n",sizeof(li));
	printf("long long %lu\n",sizeof(lli));
	printf("float %lu\n",sizeof(f));
	printf("double %lu\n",sizeof(d));
	printf("long double %lu\n",sizeof(ld));
	printf("**** ENDIANESS ****\n");
	i=1;
	char *c2=(char*)&i;
	if( *c2 ) 
		printf("Little Endian\n");
	else
		printf("Big Endian\n");
	printf("*******************\n");
}
