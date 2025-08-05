//rangecoder entropy coding. Copyright(c)Daisuke Okanohara 2002-2004. All rights reserved
#ifndef __rangecoder_cpp__
#define __rangecoder_cpp__
#include<stdio.h>
#include<iostream>
#include<vector>
#include"math.h"

using namespace std;
typedef unsigned char uc;
typedef unsigned int uint;
typedef vector<uint>::iterator ui;

#define TOP (1U << 24)
#define BOT (1U << 16)

//確率の合計が65536まで
void makeCumFreq_dec(vector<uint>& cumFreq, int shift);
void makeCumFreq(vector<uint>& freq,vector<uint>& cumFreq);
void makeCumFreq_dec(vector<uint>& freq,vector<uint>& cumFreq);
uint normalize(uint* freq, int shift, int size);
uint normalize(vector<uint>& freq, int shift);
void makeCumFreq(uint* freq, uint* cumFreq, int size);
void makeCumFreq_dec(uint* freq, uint* cumFreq, int size);

class rangeEncoder{
private:
	uint low, range;
	uint passed;	//number of output
	FILE* outfp;
	uint count;

void pc(uc c){
	fputc(c,outfp);
	passed++;
}
public:
uint getPassed(){
	return passed;
}
rangeEncoder(FILE* o): outfp(o){
	setup();
}
void setup(){
	low=passed=count=0;
	range=0xFFFFFFFF;
}
void flush(){
	for(int i = 4;i--;low <<= 8) pc(low >> 24);
}
~rangeEncoder(){
	//flush();
}
//---: normal coding----------
public: void encode(uint cumFreq, uint freq, uint totFreq){
	range /= totFreq;
	low += range * cumFreq;

	for(range *= freq;(low ^ (low + range)) < TOP || range < BOT && (range=-low&BOT-1);range <<= 8)
		pc(low >> 24), low <<= 8;
}
//---: using shift instead of dividing----------
public: void encodeshift(uint cumFreq, uint freq, uint totShift){
	range >>= totShift;
	low += range * cumFreq;

	for(range *= freq;(low ^ (low + range)) < TOP || range < BOT && (range=-low&BOT-1);range <<= 8)
		pc(low >> 24), low <<= 8;
}
//---: coding number lower than maxnumber----------
void encodeNumber (uint number, uint maxnumber){
	if(number >= maxnumber){
		printf("%d %d\n",number,maxnumber);
		throw"encodeNumber error";
	}
	encode(number,1,maxnumber);
}
void valueDebug(){
	fprintf(stderr,"%8u,%15u,%15u\n",count, low,range);
	count++;
}
};
class rangeDecoder{
private:
	uint low, code, range;
	FILE* infp;
	uint count;
	uc gc(){return fgetc(infp);}
public:
	rangeDecoder(){}
	void setup(FILE* in){
		infp = in;
		init();
	}
	void init(){
		low = code = count = 0;
		range = 0xFFFFFFFF;
		for(int i = 4;i--;)code = (code << 8) | gc();
	}
//---: normalize range and low-----------
void normalize(){
	for(;(low ^ (low + range)) < TOP || range < BOT && (range=-low&BOT-1);range <<= 8)
		code = code << 8 | gc(), low <<= 8;
}
//---: get next accumurate prob using shift----------
uint getfreqshift(uint totshift){
	range >>= totshift;
	uint tmp = (code - low) / range;
	if(tmp >= (uint)(1 << totshift)) throw"totfreq error";
	return tmp;
}
//---: get next accumurate prob----------
uint getfreq(uint totFreq){
	range /= totFreq;
	uint tmp = (code - low) / range;
	if(tmp >= totFreq) throw"totfreq error";
	return tmp;
}
uint basic_getCharacter(vector<uint>& freq,vector<uint>& cumFreq,int n){
	uint tmp = (code - low) / range;
	if(tmp >= cumFreq[n]){
		fprintf(stderr,"tmp:%d cumFreq[n]:%d totfreq error",tmp,cumFreq[n]);
		throw"basic_getCharacter error\n";
	}
	int i = 0, j = n;
	while(i < j){
		int k = i+j>>1;
		if(cumFreq[k + 1] > tmp) j = k;
		else i = k + 1;
	}
	int cf = cumFreq[i], fr = cumFreq[i+1] - cf;
	low += cf * range;
	range *= fr;
	normalize();
	return i;
}
uint basic_getCharacter2(vector<uint>& cumFreq,int n){
	uint tmp = (code - low) / range;
	if(tmp >= cumFreq[n]){
		fprintf(stderr,"tmp:%d cumFreq[n]:%d totfreq error",tmp,cumFreq[n]);
		throw"basic_getCharacter error\n";
	}
	int i = 0, j = n;
	while(i < j){
		int k = i+j>>1;
		if(cumFreq[k + 1] > tmp) j = k;
		else i = k + 1;
	}
	int cf = cumFreq[i], fr = cumFreq[i+1] - cf;
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
		throw"";
	}
	int i = 0, j = n;
	while(i < j){
		int k = i+j>>1;
		if(cumFreq[k + 1] > tmp) j = k;
		else i = k + 1;
	}
	int cf = cumFreq[i], fr = cumFreq[i+1] - cf;
	if(fr != freq[i]) printf("error\n");
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
		throw"totfreq error";
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
void valueDebug(){
	fprintf(stderr,"%8u,%15u,%15u\n",count, low,range);
	count++;
}
};
#endif