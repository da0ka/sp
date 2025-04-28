#ifndef __rangecoder_cpp__
#define __rangecoder_cpp__

/*
	rangecoder
	entropy coding
	Copyright(c) Daisuke Okanohara 2002-2004. All rights reserved.
*/

#include<stdio.h>
#include<iostream>
#include<vector>
#include"math.h"

using namespace std;
typedef unsigned char uc;
typedef unsigned int  uint;
typedef vector<uint>::iterator ui;

#define TOP (1U << 24)
#define BOT (1U << 16)
//The sum of frequency(probabilities) is up to 65536

void makeCumFreq_dec(vector<uint>& cumFreq, int shift);
void makeCumFreq(vector<uint>& freq,vector<uint>& cumFreq);
void makeCumFreq_dec(vector<uint>& freq,vector<uint>& cumFreq);
uint normalize(uint* freq, int shift, int size);
uint normalize(vector<uint>& freq, int shift);
void makeCumFreq(uint* freq, uint* cumFreq, int size);
void makeCumFreq_dec(uint* freq, uint* cumFreq, int size);

class rangeEncoder
{
private:
	uint low;
	uint range;
	uint passed;	//number of output
	FILE* outfp;

	uint count;

void pc(uc c){
	fputc(c,outfp);
	passed++;
}
public:
uint getPassed(void){
	return passed;
}
rangeEncoder(FILE* o): outfp(o){
	setup();
}
void setup(){
	low    = 0;
	passed = 0;
	range  = 0xFFFFFFFF;

	count  = 0;
}
void flush(){
	for(int i = 0; i < 4; i++){
		unsigned char t = (unsigned char)(low >> 24);
		pc(low >> 24);
		low <<= 8;
	}
}
~rangeEncoder(){
	//flush();
}
//---: normal coding----------
void encode(uint cumFreq, uint freq, uint totFreq){
	range /= totFreq;
	if(range == 0) throw "range error";
	low += cumFreq * range;
	range *= freq;

	while((low ^ (low + range)) < TOP){
		pc(low >> 24);
		range <<= 8;
		low   <<= 8;
	}
	while(range < BOT){
		pc(low >> 24);
		range = ((-(int)low) & (BOT - 1)) << 8;
		low <<= 8;
	}
}
//---: using shift instead of dividing----------
void encodeshift(uint cumFreq, uint freq, uint totShift){
	range >>= totShift;
	if(range == 0) throw "range error";
	low += cumFreq * range;
	range *= freq;

	while((low ^ (low + range)) < TOP){
		pc(low >> 24);
		range <<= 8;
		low   <<= 8;
	}
	while(range < BOT){
		pc(low >> 24);
		range = ((-(int)low) & (BOT - 1)) << 8;
		low <<= 8;
	}
}
//---: coding number lower than maxnumber----------
void encodeNumber (uint number, uint maxnumber){
	if(number >= maxnumber){
		printf("%d %d\n",number,maxnumber);
		throw "encodeNumber error";
	}
	encode(number,1,maxnumber);
}
void valueDebug(void){
	fprintf(stderr,"%8u,%15u,%15u\n",count, low,range);
	count++;
}
};
class rangeDecoder{
private:
	uint low;
	uint code;
	uint range;

	FILE* infp;

	uint count;

	uc  gc(){ return fgetc(infp);}
public:
	rangeDecoder(){}
	void setup(FILE* in){
		infp = in;
		init();
	}
	void init(){
		low  = 0;
		code = 0;
		range = 0xFFFFFFFF;
		for(int i = 0; i < 4; i++){
			code = (code << 8) | gc();
		}
		count = 0;
	}
//---: normalize range and low-----------
void normalize(){
	while((low ^ (low + range)) < TOP){
		code = (code << 8) | gc();
		range <<= 8;
		low   <<= 8;
	}
	while(range < BOT){
		code = (code << 8) | gc();
		range = ((-(int)low) & (BOT - 1)) << 8;
		low <<= 8;
	}
}
//---: get next accumurate prob using shift----------
uint getfreqshift(uint totshift){
	range >>= totshift;
	uint tmp = (code - low) / range;
	if(tmp >= (uint)(1 << totshift)) throw "totfreq error";
	return tmp;
}
//---: get next accumurate prob----------
uint getfreq(uint totFreq){
	range /= totFreq;
	uint tmp = (code - low) / range;
	if(tmp >= totFreq) throw "totfreq error";
	return tmp;
}
uint basic_getCharacter(vector<uint>& freq,vector<uint>& cumFreq,int n){
	uint tmp = (code - low) / range;
	if(tmp >= cumFreq[n]){
		fprintf(stderr,"tmp:%d cumFreq[n]:%d totfreq error",tmp,cumFreq[n]);
		throw "basic_getCharacter error\n";
	}
	int i = 0;
	int j = n;
	while(i < j){
		int k = (i + j) / 2;
		if(cumFreq[k + 1] <= tmp) i = k + 1;
		else j = k;
	}
	int cf = cumFreq[i];
	int fr = cumFreq[i+1] - cumFreq[i];

	low += cf * range;
	range *= fr;
	normalize();

	return i;
}
uint basic_getCharacter2(vector<uint>& cumFreq,int n){
	uint tmp = (code - low) / range;
	if(tmp >= cumFreq[n]){
		fprintf(stderr,"tmp:%d cumFreq[n]:%d totfreq error",tmp,cumFreq[n]);
		throw "basic_getCharacter error\n";
	}
	int i = 0;
	int j = n;
	while(i < j){
		int k = (i + j) / 2;
		if(cumFreq[k + 1] <= tmp) i = k + 1;
		else j = k;
	}
	int cf = cumFreq[i];
	int fr = cumFreq[i+1] - cumFreq[i];

	low += cf * range;
	range *= fr;
	normalize();

	return i;
}
uint getCharacterShift2(vector<uint>& cumFreq,int n, int totShift){
	range >>= totShift;
	return basic_getCharacter2(cumFreq,n);
}
uint basic_getCharacter(uint* freq, uint* cumFreq, int n){
	uint tmp = (code - low) / range;
	if(tmp >= cumFreq[n]){
		printf("tmp:%d cumFreq[n]:%d totfreq error",tmp,cumFreq[n]);
		throw "";
	}
	int i = 0;
	int j = n;
	while(i < j){
		int k = (i + j) / 2;
		if(cumFreq[k + 1] <= tmp) i = k + 1;
		else j = k;
	}
	int cf = cumFreq[i];
	int fr = cumFreq[i+1] - cumFreq[i];
	if(cumFreq[i+1] - cumFreq[i] != freq[i]){
		printf("error\n");
	}
	low += cf * range;
	range *= fr;
	normalize();

	return i;
}
uint getCharacter(vector<uint>& freq,vector<uint>& cumFreq,int n){
	range /= cumFreq[n];
	return basic_getCharacter(freq,cumFreq,n);
}
uint getCharacterShift(vector<uint>& freq,vector<uint>& cumFreq,int n, int totShift){
	range >>= totShift;
	return basic_getCharacter(freq,cumFreq,n);
}
uint getCharacterShift(uint* freq, uint* cumFreq, int n, int totShift){
	range >>= totShift;
	return basic_getCharacter(freq,cumFreq,n);
}
uint decodeNumber(uint maxNumber){
	range /= maxNumber;
	uint tmp = (code - low) / range;

	if(tmp >= maxNumber){
		printf("[error:%d %d]\n",tmp,maxNumber);
		throw "totfreq error";
	}
	low += tmp * range;
	normalize();
	return tmp;
}
void decode(uint cumfreq, uint freq){
	low += cumfreq * range;
	range *= freq;
	normalize();
}
void valueDebug(void){
	fprintf(stderr,"%8u,%15u,%15u\n",count, low,range);
	count++;
}
};
#endif