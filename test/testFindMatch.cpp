#include "interface/StandardIncludes.hpp"
#include "interface/FindMatch.hpp"

#ifndef NO_ROOT
	#include "TRandom3.h"
	#include "TMath.h"
	#include "TCanvas.h"
	#include "TH1D.h"
	#include "TLine.h"
	#include "TPad.h"
#endif


int main() 
{
#ifndef NO_ROOT
	TRandom3 r;r.SetSeed( (unsigned)time(NULL) ) ;
	FindMatch A;
	A.SetMaxWindow(0); 
	// create a basic spill random time distributions
	vector<uint64_t> t;	
	uint64_t lastTime=100;
	for(int i=0;i<2000; ++i) // ~ eventInSPill
	{
		lastTime +=  TMath::Floor(r.Uniform(5,30.) );
		t.push_back(lastTime);
	}
	vector<uint64_t> time1,time2;

	// with a certain probability k, accept time1, time2. Smear time2 guas + a constant diff
	uint64_t delta=TMath::Floor(r.Uniform(10,100) );
	for(unsigned int i=0; i<t.size(); ++i)
		{
			// 0.0005
		if (r.Uniform() > 0.0005) time1.push_back(t[i]);
		if (r.Uniform() > 0.0005) time2.push_back(
				TMath::Floor( r.Gaus(t[i],1.1) ) + delta
				);
		}
	// 
	A.SetTimes(time1,time2);
	printf("------- START -------\n");
	printf("-- >  t.size = %lu\n",t.size());
	printf("-- >  t1.size = %lu\n",time1.size());
	printf("-- >  t2.size = %lu\n",time2.size());
	printf("-- >  time unit estimation = %u\n", pow(long(time1.size()) -long(time2.size()),2) * TMath::Max(time1.size(),time2.size()) );
	uint64_t myStart=(uint64_t)time(NULL);
	int status=A.Run();
	uint64_t myStop=(uint64_t)time(NULL);
	printf("USER TIME: %u\n",(unsigned int)(myStop-myStart));
	printf("     status=%d == 0\n", status ) ;
	printf("     alpha=%lf\n", A.GetAlpha() ) ;
	printf("     Window (If SLOW REDUCE)=%d\n", A.GetMaxWindow() ) ;
	printf("     Converged=%d\n", int(A.Converged()) ) ;
	// PLOT
	{
		TCanvas *c=new TCanvas("c","c",800,800);
		TH1D *h0 =new TH1D("time" ,"time;time;time" ,1000,0,1000.);
		TH1D *h1 =new TH1D("time1","time1;time;time1",1000,0,1000.);
		TH1D *h2 =new TH1D("time2","time2;time;time2",1000,0,1000.);
		TH1D *h2_shifted =new TH1D("time2_shift","time2;time;time2",1000,0,1000.) ; h2_shifted->SetLineColor(kRed);
		TPad *p0= new TPad("p0","p0",0,0,1,0.35); p0->SetTopMargin(0); p0->SetBottomMargin(.05/.35);
		TPad *p1= new TPad("p1","p1",0,0.35,1,.65);p1->SetTopMargin(0); p1->SetBottomMargin(0);
		TPad *p2= new TPad("p2","p2",0,0.65,1,1);p2->SetTopMargin(.05/.35); p2->SetBottomMargin(0);
		p0->Draw();
		p1->Draw();
		p2->Draw();
		// fill and draw time, time1,time2
		for(unsigned int i=0;i<t.size();++i) h0->Fill( t[i] );
		for(unsigned int i=0;i<time1.size();++i) h1->Fill( time1[i] ,0.5);
		for(unsigned int i=0;i<time2.size();++i) h2->Fill( time2[i] ,0.5);
		p2->cd(); h0->Draw("HIST"); h0->GetYaxis()->SetRangeUser(0,1.2);
		p1->cd(); h1->Draw("HIST"); h1->GetYaxis()->SetRangeUser(0,1.2);
		p0->cd(); h2->Draw("HIST"); h2->GetYaxis()->SetRangeUser(0,1.2);
		c->cd();
		vector<pair<uint_t,uint_t> > matched=A.GetMatch(); // positions in time1/time2
		// compute delta
		double delta=0;
		for(uint_t i=0;i<matched.size();++i)
			delta += int64_t(time1[matched[i].first])-int64_t(time2[matched[i].second]);
		delta /= matched.size();
		for(unsigned int i=0;i< matched.size() ;i++ ) 
			{
			TLine *l=new TLine();	
			//l->SetName( Form("Line_%u",i) );
			l->SetLineWidth(2);
			uint64_t x1=time1[matched[i].first];
			uint64_t x2=time2[matched[i].second];
			h2_shifted->Fill(int64_t(x2)+delta,1);
			double max=h0->GetBinLowEdge(h0->GetNbinsX()+2);
			double min=h0->GetBinLowEdge(1);
			// LeftMargin
			double rm= p2->GetRightMargin();
			double lm= p2->GetLeftMargin();
			l->DrawLineNDC( (1.-rm-lm)*(x1-min)/(max-min)+lm,.38, 
					(1.-lm-rm)*(x2-min)/(max-min)+lm,0.07);
			}
		p0->cd();
		h2_shifted->Draw("HIST SAME");
		h2->Draw("HIST SAME"); // redraw
		c->SaveAs("Matched.png");
	}
	return 0;
#else
	printf("ROOT is Required: Recompile w/ ROOT\n");
	return 1;
#endif
}
