#include<iostream>
#include<stdio.h>
using namespace std;

void debugBitPrint(int c,int len);

class riceEncode{
	int maskn;
	//---- bit IO ----
	unsigned int bitbuf;
	int putcount;
	FILE* outfp;
public:
	riceEncode(int tn, FILE* fp): outfp(fp) , putcount(32), bitbuf(0), maskn(tn){}
	void code(int n);
	void code(int n,int mask);
	void unsignedcode(int n, int mask);	//return number of outputbit
	void unsignedcode_b(int n, int mask);
	void flush();
	void putbit(int bit);
	void putbits(int c, int len);
};

class riceDecode{
	int maskn;
	//---- bit IO ----
	unsigned int bitbuf;
	int getcount;
	FILE* infp;
public:
	riceDecode(int tn, FILE* fp): infp(fp),bitbuf(0),getcount(0),maskn(tn){}
	int decode();
	int decode(int mask);
	unsigned int unsigneddecode(int mask);
	unsigned int unsigneddecode_b(int mask);
	unsigned int getbit();
	unsigned int getbits(int n);
};