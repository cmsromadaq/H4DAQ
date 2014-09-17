#include <cstdio>
#include <cstdint>


void DrawBox(const char *str)
{
int N=0;
// printf("\u2500\u2501\n");
for(int i=0;str[i]!='\0' ;i++) N=i;
N++;
char buf[2048];

printf("\u250C") ; for(int i=0;i<N ;i++) printf("\u2500"); printf("\u2510\n");
printf("\u2502%s\u2502\n",str);
printf("\u2514") ; for(int i=0;i<N ;i++) printf("\u2500"); printf("\u2518\n");
}

int main(){
//	setlocale( LC_ALL, "en_US.UTF-8" );
	unsigned char c;
	unsigned short s;
	unsigned int i;
	unsigned long int li;
	unsigned long long int lli;
	float f;
	double d;
	long double ld;
	
	printf("**** SIZE OF ******\n");
	char buf[1023];	
	sprintf(buf,"char %lu == 1",sizeof(c)); DrawBox(buf);
	printf("short %lu\n",sizeof(s));
	printf("int %lu \n",sizeof(i));
	printf("long %lu\n",sizeof(li));
	printf("long long %lu\n",sizeof(lli));
	printf("float %lu\n",sizeof(f));
	printf("double %lu\n",sizeof(d));
	printf("long double %lu\n",sizeof(ld));
	printf("*******************\n");
	printf("max int %lu\n",sizeof(uintmax_t));
	printf("int8 %lu\n",sizeof(uint8_t));
	printf("int16 %lu\n",sizeof(uint16_t));
	sprintf(buf,"int32 %lu == 4",sizeof(uint32_t)); DrawBox(buf);
	printf("int64 %lu\n",sizeof(uint64_t));
	printf("**** ENDIANESS ****\n");
	i=1;
	char *c2=(char*)&i;
	if( *c2 ) 
		printf("Little Endian\n");
	else
		printf("Big Endian\n");
	printf("*******************\n");

}
