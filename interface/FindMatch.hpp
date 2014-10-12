
#ifndef FIND_MATCH_H
#define FIND_MATCH_H
#include "interface/StandardIncludes.hpp"

typedef unsigned int uint_t ;

class FindMatch
{
private:
	vector<uint64_t> time1;
	vector<uint64_t> time2;
	double alpha;
	int maxWindow;
	// -- test distance
	double Distance(vector<uint_t> &x,vector<uint_t> &y);
	// -- find next test element
	int FindNext(vector<uint_t> &x,int N);
	int swap(vector<bool> &x,int offset) ;

	// -- copy matched1, matched2 in Result
	int CopyResult(vector<uint_t> &x, vector<uint_t> &y);

	// -- results: distance vector
	double d2_; // 2nd min. Used to check convergence.
	double d_;
	vector<pair<uint_t,uint_t> > R_;
public:
	FindMatch(){ alpha=15.; maxWindow=1;};
	// -- data matching, pattern recognition
	// -- find match
	int Run();
	// -- set parameters
	int SetTimes(vector<uint64_t> &x, vector<uint64_t> &y);
	// --Get Results
	const double inline GetDistance(){ return d_; };
	const vector<pair<uint_t,uint_t> > inline GetMatch(){ return R_; };
	// -- Get Par
	const double inline GetAlpha(){return alpha;};
	const int inline GetMaxWindow(){return maxWindow;};
	// check if the second minimum is too close, should be of the order of 1
	const bool inline Converged(){return (d2_ - d_ ) > 0.2; } ;
	// SET
	inline void SetMaxWindow(int i){ maxWindow=i;};

};
#endif
