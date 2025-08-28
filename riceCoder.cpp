#include"riceCoder.hpp"

void debugBitPrint(int c,int len){
	for(int i = 0; i < len; i++)
		printf("%d",(c >> len - i - 1)&1);
}
unsigned int rightbits(int len,int c){
	return c & ((1U << len) - 1U);
}
void riceEncode::putbits(int c, int len){
	for(;len >= putcount;putcount = 32){
		len -= putcount;
		bitbuf |= rightbits(putcount, c >> len);
		fwrite(&bitbuf,sizeof(unsigned int),1,outfp);
		bitbuf = 0;
	}
	putcount -= len;
	bitbuf |= rightbits(len,c) << putcount;
}
void riceEncode::putbit(int bit){
	putcount--;
	if(bit) bitbuf |= (1 << putcount);
	if(putcount == 0){
		fwrite(&bitbuf,sizeof(unsigned int),1,outfp);
		bitbuf = 0;
		putcount = 32;
	}
}
void riceEncode::flush(){
	putbits(0,31);
}
unsigned int riceDecode::getbit(){
	if(--getcount >= 0) return (bitbuf >> getcount) & 1U;
	getcount = 31;
	fread(&bitbuf,sizeof(unsigned int),1,infp);
	return(bitbuf >> 31) & 1U;
}
unsigned int riceDecode::getbits(int n){
	unsigned int x=0;
	for(;n > getcount;getcount = 32){
		n -= getcount;
		x |= rightbits(getcount, bitbuf) << n;
		fread(&bitbuf,sizeof(unsigned int),1,infp);
	}
	getcount -= n;
	return x | rightbits(n,bitbuf >> getcount);
}
void riceEncode::code(int n){
	putbit(n<0);
	if(n<0) n = ~n;
	int b=0;
	for(;n>>++b;);
	unsignedcode(b-1,maskn);
	b>1?putbits(n,b-1):putbit(n>0);
}
void riceEncode::code(int n, int mask){
	putbit(n<0);
	if(n<0) n = ~n;
	int b=0;
	for(;n>>++b;);
	unsignedcode(b-1,mask);
	b>1?putbits(n,b-1):putbit(n>0);
}
void riceEncode::unsignedcode(int n, int mask){
	if(n < 0){
		fprintf(stderr,"%d\n",n);
		throw"unsinged code error";
	}
	int middle = n >> mask;
	putbits(0,middle); // 000...000
	putbit(1);
	putbits(n,mask);
}
int riceDecode::decode(){
	int m=getbit(), zeros = 0;
	while(!getbit()) zeros++;

	int lsb = getbits(maskn), n = (zeros << maskn) + lsb;
	if(m) n = ~n;
	return n;
}
int riceDecode::decode(int mask){
	int m=getbit(), n = unsigneddecode(mask);
	n = n? 1<<n|getbits(n): getbit();
	if(m) n = ~n;
	return n;
}
unsigned int riceDecode::unsigneddecode(int mask){
	int zeros = 0;
	while(!getbit()) zeros++;
	return(zeros << mask) + getbits(mask);
}