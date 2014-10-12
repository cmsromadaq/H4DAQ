#include "interface/FindMatch.hpp"
//#define FM_DEBUG

int FindMatch::SetTimes(vector<uint64_t> &x,vector<uint64_t> &y)
{
	time1=x; // copy
	time2=y; 
	return 0;
}

double FindMatch::Distance(vector<uint_t> &x, vector<uint_t> &y)
{
	// not implemented all the checks. Not intended for general public
	if ( x.size() != y.size() ) return 1.e100;
	if ( x.size() <=2 ) return 1.e100;
	// transform back to 0100010
	double R=0;
	// compute delta medio
	double delta=0;
	for(uint_t i=0;i<x.size();++i)
		{
		delta += int64_t(time1[x[i]])-int64_t(time2[y[i]]);
		}
	delta /= x.size();
	// 
	for(uint_t i=0;i<x.size();++i)
	{
		R+= sqr( (  int64_t(time1[x[i]])-int64_t(time2[y[i]])-delta) ); // ???
		if ( R> d2_ ) return R; // speed up // to further speed up, this have to branch out FindNext
	}
	//R /= (x.size()-1);

	uint_t min=time1.size();
	if (time2.size() <min) min=time2.size() ;
	min-= x.size();
	R += alpha*min;

#ifdef FM_DEBUG
	if( R < 10 ) printf("--> delta=%lf D=%lf PEN=%lf\n",delta,R,alpha*min);
#endif
	return R;
	
}

int FindMatch::FindNext(vector<uint_t> &x,int N)
{
// re-arrange element of x \in [0,N-1] in order to find the next element in the search
// each element should be sorted x[i] < x[i+i], but some x[i] could miss
// starts from 1...N, returns 0 in case of success, 1 if last is provided
// return -1 (<0) if error
 uint_t M=x.size();
 if (M>N) return -1;
 // check if is last
 bool isLast=true;
 for (uint_t i=0;i<M && isLast;i++)
	if ( x[i] != N-M+i) 
	{
//#ifdef FM_DEBUG
//		printf("NOT LAST: i=%u x[i] = %u !=  (N-i-1) %u\n",i,x[i],N-i-1);
//#endif
		isLast=false;
	}
 if ( isLast ) return 1;

 vector<bool> pos;
 pos.resize(N,false);
//#ifdef FM_DEBUG
//	printf("M=%u >= N=%u \nX[i]=",M,N);
//#endif
 for( uint_t i=0;i<M;++i)pos[i] =false;
 for( uint_t i=0;i<M;++i)
 	{
//#ifdef FM_DEBUG
//	printf("%u - ",x[i]);
//#endif
	pos[ x[i] ] = true;
	}
//#ifdef FM_DEBUG
//	printf("\n");
//#endif
 // find first (1,0) and put it (0,1)
//#ifdef FM_DEBUG
// printf("OLD:");
// for( uint_t i=0;i<N;++i)
//	 if (pos[i]) printf("1");
//	 else printf("0");
// printf("\n");
//#endif
 if( swap(pos,0)> 0 ) return -2 ; // if return 1, I'm using all of them, first==last, so should not be here
#ifdef FM_DEBUG
 printf("NEW:%d:",N);
 for( uint_t i=0;i<N;++i)
	 if (pos[i]) printf("1");
	 else printf("0");
 printf("\n");
#endif
 // re-assign x
 x.clear(); x.reserve(M);
 for(uint_t j=0;j< pos.size();++j)
	{
	if(pos[j]) x.push_back(j);
 	}
 if (x.size() != M ) return -3; // should never happen. Use for debug
 return 0;
}


// 0 = swapped ; 1 = not swapped
int FindMatch::swap(vector<bool> &x,int offset) {
	if (offset >= x.size() -1 ) return 1; // not swap
	if ( swap(x,offset+1)==0 ) return 0;

	// I didn't swap	
	if (x[offset] == x[offset+1] ) return 1; // they are equal 00 11
	// they are differet but 01
	else if (x[offset] == false && x[offset+1] == true ) return 2;
	// they are differet 10 -> 01
	else {
		bool tmp= x[offset];
		x[offset]=x[offset+1];
		x[offset+1]=tmp;
		// put following 0s to the end
		uint_t last;
		for(last =x.size() -1 ; last > offset+2 && !x[last]  ;last -- );

		for( uint_t i=offset+2; i<last ;++i) // if i
			{
			if (x[i] == false)
				{ // swap with last
				bool tmp= x[i];
				x[i]=x[last];
				x[last]=tmp;
				//update last
				for(last =x.size() -1 ; last > offset+2 && !x[last]  ;last -- );
				}
			}
		return 0;
	}
}

int FindMatch::CopyResult(vector<uint_t> &x, vector<uint_t> &y)
{
	if (x.size() != y.size() ) return 1;
	R_.clear(); R_.reserve(x.size() );
	for(uint_t i=0;i<x.size() ;++i) 
		R_.push_back(pair<uint_t,uint_t>(x[i],y[i]) );
	return 0;
}

int FindMatch::Run()
{
// Gen Permutations
uint_t minSize=time1.size();
if (time2.size() < minSize ) minSize=time2.size();

// construct the two results vectors and init them
vector<uint_t> matched1, matched2;
//initial permutation
d_=1e100;
d2_=1e100;
for (int w=0;w <= maxWindow ;++w)
	{
	// reset matched1
	matched1.clear();matched1.reserve(minSize-w);
	for(uint_t i=0;i<minSize-w;++i ) 
		matched1.push_back(i);
	// Find all permutations of matched1/2
	int status1=0;
	int status2=0;
	do  // combination time1
	{
	//reset matched2
	matched2.clear();matched2.reserve(minSize-w);
	for(uint_t i=0;i<minSize-w;++i ) 
		matched2.push_back(i);
	do // combination 2
		{
//  #ifdef FM_DEBUG
//  		printf("[Run]::[DEBUG] Running matched1 (time1:%lu) :matched2 (time2:%lu)\n",time1.size(),time2.size());
//  		for(int i=0;i<matched1.size() ;i++ ) printf("%u - ", matched1[i]);
//  		printf("\n");
//  		for(int i=0;i<matched2.size() ;i++ ) printf("%u - ", matched2[i]);
//  		printf("\n");
//  #endif
		double d1=Distance(matched1,matched2);
		if(d1<d_)
			{
			d2_=d_;
			d_=d1;
			CopyResult(matched1,matched2);
			}
		}while ( (status2=FindNext(matched2,time2.size())) == 0 ); //end while combination 2
	if (status2< 0 ) return -200 + status2;	
	}while (  (status1=FindNext(matched1,time1.size()) ) == 0 ); //end while combination 1
	if (status1< 0 ) return -100 + status1;	
	} // end for
return 0;
}
