/*
	sp
	Daisuke Okanohara (VZV05226@nifty.com)
*/
#include"riceCoder.hpp"

void debugBitPrint(int c,int len){
	for(int i = 0; i < len; i++){
		printf("%d",(c >> ((len - i) - 1))&1);
	}
}
unsigned int rightbits(int len,int c){
	return ((c) & ((1U << (len)) - 1U));
}
void riceEncode::putbits(int c, int len){
	while(len >= putcount){
		len -= putcount;
		bitbuf |= rightbits(putcount, c >> len);
		fwrite(&bitbuf,sizeof(unsigned int),1,outfp);
		bitbuf = 0;
		putcount = 32;
	}
	putcount -= len;
	bitbuf |= rightbits(len,c) << putcount;
}
void riceEncode::putbit(int bit){
	putcount--;
	if(bit != 0) bitbuf |= (1 << putcount);
	if(putcount == 0){
		fwrite(&bitbuf,sizeof(unsigned int),1,outfp);
		bitbuf = 0;
		putcount = 32;
	}
}
void riceEncode::flush(){
	putbits(0,31);
}
unsigned int  riceDecode::getbit(void){
	if(--getcount >= 0) return (bitbuf >> getcount) & 1U;
	getcount = 31;

	fread(&bitbuf,sizeof(unsigned int),1,infp);

	return (bitbuf >> 31) & 1U;
}
unsigned int  riceDecode::getbits(int n){
	unsigned int x;
	x = 0;
	while(n > getcount){
		n -= getcount;
		x |= rightbits(getcount, bitbuf) << n;
		fread(&bitbuf,sizeof(unsigned int),1,infp);
		getcount = 32;
	}
	getcount -= n;
	return x | rightbits(n,bitbuf >> getcount);
}
void riceEncode::code(int n){
	if(n >= 0){
		putbit(0);
	}
	else{
		putbit(1);
		n = -n;
		n--;
	}
	int middle = n >> maskn;
	putbits(0,middle); // 000...000
	putbit(1);
	putbits(n,maskn);
}
void riceEncode::code(int n, int mask){
	if(n >= 0){
		putbit(0);
	}
	else{
		putbit(1);
		n = -n;
		n--;
	}
	int middle = n >> mask;
	putbits(0,middle); // 000...000
	putbit(1);
	putbits(n,mask);
}
void riceEncode::unsignedcode(int n, int mask){
	if(n < 0){
		fprintf(stderr,"%d\n",n);
		throw "unsinged code error";
	}
	int middle = n >> mask;
	putbits(0,middle); // 000...000
	putbit(1);
	putbits(n,mask);
}
void riceEncode::unsignedcode_b(int n, int mask){
	putbits(n,mask);
}
int riceDecode::decode(){
	bool plus;
	if(getbit() == 0){
		plus = true;
	}
	else{
		plus = false;
	}
	int zeros = 0;
	while(getbit() != 1) zeros++;

	int lsb = getbits(maskn);

	int n = (zeros << maskn) + lsb;
	if(plus == false){
		n++;
		n = -n;
	}
	return n;
}
int riceDecode::decode(int mask){
	bool plus;
	if(getbit() == 0){
		plus = true;
	}
	else{
		plus = false;
	}
	int zeros = 0;
	while(getbit() != 1) zeros++;

	int lsb = getbits(mask);

	int n = (zeros << mask) + lsb;
	if(plus == false){
		n++;
		n = -n;
	}
	return n;
}
unsigned int riceDecode::unsigneddecode(int mask){
	int zeros = 0;
	while(getbit() != 1) zeros++;

	int lsb = getbits(mask);

	unsigned int n = (zeros << mask) + lsb;
	return n;
}
unsigned int riceDecode::unsigneddecode_b(int mask){
	return getbits(mask);
}
