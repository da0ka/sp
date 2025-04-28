/*
	sp
	Daisuke Okanohara (VZV05226@nifty.com)
*/
#include"rangecoder.hpp"

//typedef long long int unsigned __int64;

//---: Normalize as Sigma{freq} = 1 << shift----------
uint normalize(vector<uint>& freq, int shift){
	int freqMax = 1 << shift;
	int total = 0;

	for(ui p = freq.begin(); p != freq.end(); p++){
		total += *p;
	}
	if(total == 0) return 0;

	int total2 = 0;
	int kind   = 0;
	for(ui p = freq.begin(); p != freq.end(); p++){
		if(*p > 0){
			long long int t = (long long int)*p;
			t *= freqMax;

			if(t >= (int)total){
				t /= total;

				*p = (uint)t;
			}
			else{
				*p = 1;
			}
			total2 += *p;
			kind   ++;
		}
	}
	//---: adjust-----------
	if(total2 > freqMax){
		//Decreasing each frequency
		int sub = total2 - freqMax;

		//DEBUG
		if(sub > kind){
			printf("error sub:%d kind:%d\n",sub,kind);
		}
		while(sub){
			for(ui p = freq.begin(); p != freq.end(); p++){
				if(*p >= 2){
					*p = *p - 1;
					sub--;
					if(sub == 0) break;
				}
			}
		}
	}
	else if(total2 < freqMax){
		//---: Increasing each frequency-----------
		int sub = freqMax - total2;
		while(sub){
			for(ui p = freq.begin(); p != freq.end(); p++){
				if(*p > 0){
					*p = *p + 1;
					sub --;
					if(sub == 0) break;
				}
			}
		}
	}
	return total;
}
void makeCumFreq_dec(vector<uint>& cumFreq, int shift){
	cumFreq[cumFreq.size() - 1] = 0;
	int freqMax = 1 << shift;
	int total = 0;

	for(ui p = cumFreq.begin(); p != cumFreq.end(); p++){
		total += *p;
	}
	if(total == 0) return;

	int total2 = 0;
	int kind   = 0;
	for(ui p = cumFreq.begin(); p != cumFreq.end(); p++){
		if(*p > 0){
			long long int t = (long long int)*p;
			t *= freqMax;

			if(t >= (int)total){
				t /= total;

				*p = (uint)t;
			}
			else{
				*p = 1;
			}
			total2 += *p;
			kind   ++;
		}
	}
	if(total2 > freqMax){
		int sub = total2 - freqMax;

		//DEBUG
		if(sub > kind){
			printf("error sub:%d kind:%d\n",sub,kind);
		}
		while(sub){
			for(ui p = cumFreq.begin(); p != (cumFreq.end() - 1); p++){
				if(*p >= 2){
					*p = *p - 1;
					sub--;
					if(sub == 0) break;
				}
			}
		}
	}
	else if(total2 < freqMax){
		int sub = freqMax - total2;
		while(sub){
			for(ui p = cumFreq.begin(); p != (cumFreq.end() - 1); p++){
				if(*p > 0){
					*p = *p + 1;
					sub --;
					if(sub == 0) break;
				}
			}
		}
	}
	//set cumFreq
	int sum = 0;
	for(vector<uint>::iterator i = cumFreq.begin(); i != cumFreq.end(); i++){
		sum += *i;
		*i = sum - *i;
	}
}
uint normalize(uint* freq, int shift, int size){
	int freqMax = 1 << shift;
	int total = 0;

	for(int i = 0; i < size; i++){
		total += freq[i];
	}
	if(total == 0) return 0;

	int total2 = 0;
	int kind   = 0;
	for(int i = 0; i < size; i++){
		long long int t = (long long int)freq[i];
		t *= freqMax;

		if(t >= (int)total){
			t /= total;
			freq[i] = (uint)t;
		}
		else{
			freq[i] = 1;
		}
		total2 += freq[i];
		kind   ++;
	}
	if(total2 > freqMax){
		int sub = total2 - freqMax;

		if(sub > kind){
			printf("error sub:%d kind:%d\n",sub,kind);
		}
		while(sub){
			for(int i = 0; i < size; i++){
				if(freq[i] >= 2){
					freq[i]--;
					sub--;
					if(sub == 0) break;
				}
			}
		}
	}
	else if(total2 < freqMax){
		int sub = freqMax - total2;
		while(sub){
			for(int i = 0; i < size; i++){
				freq[i]++;
				sub --;
				if(sub == 0) break;
			}
		}
	}
	return total;
}
void makeCumFreq_dec(vector<uint>& freq,vector<uint>& cumFreq){
	if((freq.size() + 1) != cumFreq.size()){
		throw "freq cumFreq error";
	}
	if(freq.size() == 0){
		throw "freq is zero";
	}
	int size = (int)freq.size();
	for(int i = 0; i < size; i++){
		cumFreq[i + 1] = freq[i] + cumFreq[i];
	}
}
void makeCumFreq_dec(uint* freq, uint* cumFreq, int size){
	cumFreq[0] = 0;
	for(int i = 0; i < size; i++){
		cumFreq[i + 1] = freq[i] + cumFreq[i];
	}
}
void makeCumFreq(vector<uint>& freq,vector<uint>& cumFreq){
	if(freq.size() != cumFreq.size() || freq.size() == 0){
		fprintf(stderr,"%d %d\n",freq.size(),cumFreq.size());
		throw "freq cumFreq error";
	}
	int size = (int)freq.size();
	for(int i = 1; i < size; i++){
		cumFreq[i] = freq[i - 1] + cumFreq[i - 1];
	}
}
void makeCumFreq(uint* freq, uint* cumFreq, int size){
	cumFreq[0] = 0;
	for(int i = 1; i < size; i++){
		cumFreq[i] = freq[i - 1] + cumFreq[i - 1];
	}
}
