#include "interface/VFE_adapter.hpp"

#include <getopt.h>
#include <chrono>

void usage(const char * prg)
{
	printf("Usage: %s <options>\n", prg);
	printf("where option can be:\n");
	printf(" -c, --cfg <file>,  where <file> is a xml configuration file [default: `data/VFE_adapter.xml'\n");
	printf(" -r, --root <file>, produces a root tree, where <file> is the output file name\n");
	printf(" -t, --time,        measure the readout rate (pretty useless if in conjunction with -r)\n");
	exit(1);
}


typedef unsigned long timestamp_t;
typedef unsigned short int sample_t;
typedef struct {
        int         _id;
        timestamp_t _ts;
        sample_t * _samples[5];
} Event;
Event e_;


void init_branches(TTree * t, int nsamples)
{
        t->Branch("id", &e_._id, "id/I");
        t->Branch("timestamp", &e_._ts, "timestamp/l");
        for (int i = 0; i < 5; ++i) {
                char tmp1[64], tmp2[64];
                sprintf(tmp1, "ch%d", i);
                sprintf(tmp2, "ch%d[%d]/s", i, nsamples);
                t->Branch(tmp1, e_._samples[i], tmp2);
        }
}


void dump_event(TTree * t, std::vector<WORD> & w, int ndevices, int nsamples)
{
        for (int id = 0; id < ndevices; ++id) {
                e_._id = id;
                if((w[0]>>31) != 1) printf("Sample 0 not a header : %8.8x\n",w[0]);
                unsigned long int t1= w[0]     &0xFFFF;
                unsigned long int t2= w[1]     &0xFFFF;
                unsigned long int t3=(w[1]>>16)&0xFFFF;
                unsigned long int t4= w[2]     &0xFFFF;
                unsigned long int t5=(w[2]>>16)&0x00FF;
                e_._ts = (t5<<56) + (t4<<42) + (t3<<28) + (t2<<14) + t1;

                int debug = 0;

                if(debug > 0)
                {
                        printf("timestamp : %8.8x %8.8x %8.8x\n", w[2], w[1], w[0]);
                        printf("timestamp : %ld %4.4x %4.4x %4.4x %4.4x %4.4x\n", e_._ts, t5, t4, t3, t2, t1);
                }
                for(int is = 0; is < nsamples; ++is)
                {
                        int j=(is+1)*3;
                        e_._samples[0][is]= w[j]       &0xFFFF;
                        e_._samples[1][is]= w[j+1]     &0xFFFF;
                        e_._samples[2][is]=(w[j+1]>>16)&0xFFFF;
                        e_._samples[3][is]= w[j+2]     &0xFFFF;
                        e_._samples[4][is]=(w[j+2]>>16)&0xFFFF;
                        if(debug > 0) {
                                printf("%8.8x %8.8x %8.8x\n", w[j], w[j+1], w[j+2]);
                                printf("--> sample: %5d  channels: %8d %8d %8d %8d %8d\n", is, e_._samples[0][is], e_._samples[1][is], e_._samples[2][is], e_._samples[3][is], e_._samples[4][is]);
                        }
                }
                t->Fill();
        }
}


int main(int argc, char ** argv)
{
	if (argc == 1) usage(argv[0]);

	char cfg_file[256]  = "data/VFE_adapter.xml";
	char root_file[256];
	int compute_time = 0, root_output = 0;

        int c;
	while (1) {

		static struct option long_options[] =
		{
			{"cfg",     required_argument, 0, 'c'},
			{"root",    required_argument, 0, 'r'},
			{"time",    no_argument, &compute_time, 1},
		};

		int option_index = 0;
		c = getopt_long(argc, argv, "c:r:t", long_options, &option_index);
		if (c == -1) break;

		switch (c) {
			case 0:
				if (long_options[option_index].flag != 0) break;
				printf("option %s", long_options[option_index].name);
				if (optarg) printf(" with arg %s", optarg);
				printf("\n");
			case 'c':
				sprintf(cfg_file, "%s", optarg);
                                break;
			case 'r':
                                root_output = 1;
				sprintf(root_file, "%s", optarg);
                                break;
                        default:
                                usage(argv[0]);
                }
        }

        //printf("--> cfg file: %s\n", cfg_file);
        //printf("--> root output? %d  output root file: %s\n", root_output, root_file);
        //return 0;

        // parse options

        Board * b = new VFE_adapter(); 

	Configurator * cfg = new Configurator();
	cfg->xmlFileName = "data/VFE_adapter.xml";
	//cfg->xmlFileName = argv[1];
	cfg->Init();

	// find the board in the cfg
	xmlNode * hw_node = NULL;
	for (hw_node = cfg->root_element->children; hw_node; hw_node = hw_node->next) {
		if (hw_node->type == XML_ELEMENT_NODE &&
		    xmlStrEqual (hw_node->name, xmlCharStrdup ("Hardware"))) {
			break;
                }
	}
	xmlNode * board_node = NULL;
	for ( board_node = hw_node->children; board_node; board_node = board_node->next) {
		if (board_node->type == XML_ELEMENT_NODE
		    && xmlStrEqual (board_node->name, xmlCharStrdup ("board")) ) {
			int id = Configurator::GetInt(Configurable::getElementContent (*cfg, "ID" , board_node));
                        std::string type = Configurable::getElementContent (*cfg, "type" , board_node);
                        printf("--> Configuring board id = %d   type = %s\n", id, type.c_str());
			if (type == "VFE_adapter") break;
		}
	}

	BoardConfig bc;
	bc.Init (*cfg);
	bc.SetBoardNode(board_node);

        b->Config(&bc);
        b->Init();

        b->Clear();

        TFile * froot = 0;
        TTree * t;
        int nsamples = 0, ndevices = 0;
        if (root_output) {
                froot = TFile::Open(root_file, "recreate");
                if (!froot->IsOpen()) {
                        fprintf(stderr, "Error: cannot open file `%s'\n", root_file);
                        return 2;
                }
                nsamples = (dynamic_cast<VFE_adapter *>(b))->NSamples();
                ndevices = (dynamic_cast<VFE_adapter *>(b))->NDevices();
                // 5 channels per VFE
                //e_._samples = (sample_t **)calloc(5, sizeof(sample_t *));
                for (size_t i = 0; i < 5; ++i) {
                        e_._samples[i] = (sample_t *)calloc(nsamples, sizeof(sample_t));
                }
                t = new TTree("event", "event");
                init_branches(t, nsamples);
        }

        std::vector<WORD> v;
        int cnt = 0;
        auto begin = std::chrono::high_resolution_clock::now();
        // can be commented if a fake trigger is generated
        while (cnt++ < 100) {
        //while (1) {
                // can be commented if a fake trigger is generated
                //usleep(5000);
                // uncomment to generate fake trigger
                v.clear();
                (dynamic_cast<VFE_adapter *>(b))->Trigger();
                b->Read(v);
                if (root_output) dump_event(t, v, ndevices, nsamples);
                //getchar();
        }
        if (root_output) {
                froot->Write();
                froot->Close();
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto dt = std::chrono::duration_cast<std::chrono::nanoseconds>(end-begin).count();
        printf("Elapsed time for %d events: %ld ns (%.2f Hz)\n", cnt - 1, dt, (cnt - 1) / (dt * 1e-9));

        return 0;
}
