//sp
//Daisuke Okanohara (VZV05226@nifty.com)
#include<iostream>
#include<stdio.h>
#include<string>
#include<math.h>
#include<vector>
#include<fstream>
#include<cstring>
#include<climits>
#include<float.h>
#include"rangecoder.hpp"
#include"riceCoder.hpp"

typedef vector<string> vs;
typedef unsigned int uint;
typedef unsigned char BYTE;

#define INTERVAL 256
#define MAXBUF 1048576
#define RICE_MASK 3
#define MAXDEPTH 10	//for debug

#define EXT ".d"
#define R_SHIFT 14
#define VERSION "0.2"
#define CACHE 0

namespace sp{
using namespace std;

void encode(string& fileName, vs& opt);
void decode(string& fileName, vs& opt);

struct encodeOption{
public:
	encodeOption(vs& opt):interval(INTERVAL){
		for(vs::iterator i = opt.begin(); i != opt.end(); i++)
			if((*i)[0] == 'i')
				interval = atoi((i->substr(1)).c_str());
	}
	int interval;
};
int riceUnsignedN(unsigned int n, int mask);

extern int m_number;

struct node{
	int oriTotalFreq;	//original total of frequency
	int totalFreq;		//after ignoring lower bit
	vector<BYTE> nc;
	vector<uint> oriFreq, freq, cumFreq;
	vector<node*> children, ncSkip;

	BYTE ch;			//character attched to the branch to the parent.
	node* parent;		//pointer to the parent
	int size;
	node(int* i_freq,int depth,node* parent,BYTE c);
	node* meargedNode;
	bool mearged;		//is it mearged
	bool used;			//is it used at coding
	bool initialized;	//is it initialized
	int meargedNumber;	//number of mearged node

	float kl_divergence(node* par);
	float kl_divergence2(node* par);
	int checkBitN(node* par);
	int checkBitN2();
	int checkBitN3();
	int getSize();
	int getNonMeargeSize();
	int getUsedSize();

	void outputRice(riceEncode& rc,bool m);	//m is set if node is mearged node
	void outputRice4(node* parent, rangeEncoder& ra, bool m);
	void outputRice2(node* parent, riceEncode& rc);
	void outputRice3(node* parent, riceEncode& rc);
	void set_ncSkip2();
	node* searchPath(BYTE* buf, int p, int historyLimit);

	void mearge();
	void free();

	float kl_div;
	int toBit, depth;
};
extern int totalNumber;
struct d_node{
	d_node(){}
	~d_node(){}
	int size;
	vector<BYTE> nc;
	vector<uint> cumFreq;
	vector<d_node*> ncSkip, children;
	d_node* parent;
	BYTE ch;
	int depth, No;

	d_node* searchPath(BYTE* buf, int p, int historyLimit);

	void inputRice(riceDecode& rd,int depth);
	void inputRice4(d_node* parent,rangeDecoder& rd);

	void set_ncSkip2();
	void freeChildren();
	void freeAll();
	int debugSize();
	int allocateSize();
};
class eModel{
public:
	eModel(int i_fileSize, int i_bufSize, encodeOption& eo):
	fileSize(i_fileSize),
	bufSize(i_bufSize),
	interval(eo.interval){
		//allocate memory
		printf("bufSize:%d\n",bufSize);
		buf = new BYTE[bufSize];
		if(!buf) throw"buf allocate error";
		pos = new int[bufSize];
		if(!pos) throw"pos allocate error";
		freq = new int[256];
		if(!freq) throw"freq allocate error";
		if(interval>fileSize) interval=fileSize;
		if(interval>MAXBUF) interval=MAXBUF;
	}
	~eModel(){
		if(buf) delete[] buf;
		if(pos) delete[] pos;
		if(freq) delete[] freq;
	}
	void compress(FILE* infp,FILE* outfp);
private:
	BYTE* buf;
	int*pos, *work, *freq;;
	int interval;
	int bufSize;	//processing size
	int fileSize;
	node* makeModel();
	void printDebug(node* root);
	void makeTree(node* parent, int start, int last, int depth);
};
d_node* readTree(FILE* infp, int filePos,d_node* base, BYTE* history);
}