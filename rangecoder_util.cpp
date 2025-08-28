#include"rangecoder.hpp"
//typedef long long int unsigned __int64;

//---: Normalize as Sigma{freq} = 1 << shift----------
uint normalize(vector<uint>& freq, int shift){
	int freqMax = 1 << shift, total = 0;
	ui b = freq.begin(), e=freq.end();
	for(ui p = e, q=b; p != q;) total += *--p;
	if(total == 0) return 0;

	int total2 = 0, kind = 0;
	for(ui p = e, q=b; p != q;)
		if(*--p > 0){
			long long int t = (long long int)*p;
			t *= freqMax;

			if(t >= (int)total){
				t /= total;
				*p = (uint)t;
			}else *p = 1;
			total2 += *p;
			kind++;
		}
	//---: adjust-----------
	if(total2 > freqMax){
		//Decreasing each frequency
		int sub = total2 - freqMax;

		//DEBUG
		if(sub > kind) printf("error sub:%d kind:%d\n",sub,kind);
		while(sub)
			for(ui p = b, q=e; p != q; p++)
				if(*p>1){
					--*p;
					if(--sub == 0) break;
				}
	}else if(total2 < freqMax){
		//---: Increasing each frequency-----------
		int sub = freqMax - total2;
		while(sub)
			for(ui p = b, q=e; p != q; p++)
				if(*p > 0){
					++*p;
					if(--sub == 0) break;
				}
	}
	return total;
}
void makeCumFreq_dec(vector<uint>& cumFreq, int shift){
	cumFreq[cumFreq.size()-1] = 0;
	int freqMax = 1 << shift, total = 0;
	ui b = cumFreq.begin(), e = cumFreq.end();

	for(ui p = e; p != b;) total += *--p;
	if(total == 0) return;

	int total2 = 0, kind = 0;
	for(ui p = e; p != b;)
		if(*--p > 0){
			long long int t = (long long int)*p;
			t *= freqMax;
			if(t >= (int)total){
				t /= total;
				*p = (uint)t;
			}else *p = 1;
			total2 += *p;
			kind++;
		}
	if(total2 > freqMax){
		int sub = total2 - freqMax;
		//DEBUG
		if(sub > kind) printf("error sub:%d kind:%d\n",sub,kind);
		while(sub)
			for(ui p = b; p != e; p++)
				if(*p>1){
					--*p;
					if(--sub == 0) break;
				}
	}else if(total2 < freqMax)
		for(int sub = freqMax - total2;sub;)
			for(ui p = b; p != e; p++)
				if(*p > 0){
					++*p;
					if(--sub == 0) break;
				}
	//set cumFreq
	for(int sum = 0;b != e;++b) sum += *b, *b = sum - *b;
}
uint normalize(uint* freq, int shift, int size){
	int freqMax = 1 << shift, total = 0;

	for(int i = size;i;) total += freq[--i];
	if(total == 0) return 0;

	int total2 = 0, kind = 0;
	for(int i = size;i;){
		long long int t = (long long int)freq[--i];
		t *= freqMax;

		if(t >= (int)total){
			t /= total;
			freq[i] = (uint)t;
		}else freq[i] = 1;
		total2 += freq[i];
		kind++;
	}
	if(total2 > freqMax){
		int sub = total2 - freqMax;

		if(sub > kind) printf("error sub:%d kind:%d\n",sub,kind);
		while(sub)
			for(int i = 0; i < size; i++)
				if(freq[i]>1){
					freq[i]--;
					if(--sub == 0) break;
				}
	}else if(total2 < freqMax)
		for(int sub = freqMax - total2;sub;)
			for(int i = 0; i < size; i++){
				if(freq[i]){// or always true?
					freq[i]++;
					if(--sub == 0) break;
				}
			}
	return total;
}
void makeCumFreq_dec(vector<uint>& freq,vector<uint>& cumFreq){
	int size = (int)freq.size();
	if(~size+cumFreq.size()) throw"freq cumFreq error";
	if(!size) throw"freq is zero";
	for(int i = 0; i < size; i++) cumFreq[i+1] = freq[i] + cumFreq[i];
}
void makeCumFreq_dec(uint* freq, uint* cumFreq, int size){
	for(int i = cumFreq[0] = 0; i < size; i++)
		cumFreq[i+1] = freq[i] + cumFreq[i];
}
void makeCumFreq(vector<uint>& freq,vector<uint>& cumFreq){
	int size = (int)freq.size();
	if(size != cumFreq.size() || !size){
		fprintf(stderr,"%d %d\n",size,cumFreq.size());
		throw"freq cumFreq error";
	}
	for(int i = 0, c=cumFreq[0]; ++i < size;) cumFreq[i] = c+=freq[i-1];
}
void makeCumFreq(uint* freq, uint* cumFreq, int size){
	for(int i = cumFreq[0] = 0, c=0;++i < size;)
		cumFreq[i] = c+=freq[i-1];
}