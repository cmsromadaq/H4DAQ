#include "interface/AsyncUtils.hpp"

// --- Constructor
AsyncUtils::AsyncUtils(){
	async_=false;
}
// --- Destructor
AsyncUtils::~AsyncUtils(){}

// --- Init Static Members
int AsyncUtils::maxN=20;
vector<pid_t> AsyncUtils::children;


// --- 
int AsyncUtils::GetN(){
	int N=0;
	for(int i=0;i<children.size();i++)
	{
	int status;
	pid_t pid=waitpid( children[i],  &status, WNOHANG);
	if (pid >0 ) {
		// child exited -> remove from list
		children.erase(children.begin()+i);
		}
	else if (pid == 0 ) {
		// child running
		++N;
		}
	else // pid <0
		{
		// child already terminated
		if (errno == ECHILD)
			children.erase(children.begin()+i);
		}
	}
	return N;
}

// --- Fork and register childrens
pid_t AsyncUtils::Fork(){
	if ( !async_ ) return -1;
	if ( GetN() >= maxN ) 
		{
		async_=false; // stop async
		return -1;
		}
	pid_t mypid;
	mypid=fork();
	if (mypid > 0) // parent
		{
		children.push_back(mypid);
		}
	else if (mypid==0) // children
		{
		}
	else throw fork_exception();

	return mypid;
}


