/*
	sp
	Daisuke Okanohara (VZV05226@nifty.com)
*/
#include"sp.hpp"

namespace sp{
	void d_node::inputRice(riceDecode& rc,int d){
		depth = d;
		No    = totalNumber++;
		size = rc.unsigneddecode(RICE_MASK);
		nc.resize(size);

		if(rc.getbit() == 1){
			cumFreq.resize(size + 1);

			if(size == 1){
				cumFreq[0] = 1;
			}
			else{
				for(int i = 0; i < size; i++){
					int k = rc.unsigneddecode(2) + 1;
					//fprintf(stderr,"%d\n");
					if(k > 2){
						int k1 = (k / 2);
						int k2 = (k / 2) + (k & 1);
						//fprintf(stderr,"%d\n",k1);
						int p  = rc.unsigneddecode_b(k1);

						cumFreq[i] = (p << k2);
						//fprintf(stderr,"%d\n",p);
					}
					else if(k == 2){
						cumFreq[i] = 2;
					}
					else if(k == 1){
						cumFreq[i] = 1;
					}
					//fprintf(stderr,"%d\n",freq[i]);
				}
			}
			makeCumFreq_dec(cumFreq,R_SHIFT);
		}
		if(rc.getbit()){
			int childrenCount = rc.unsigneddecode(RICE_MASK);
			children.resize(childrenCount);
			for(int i = 0; i < childrenCount; i++){
				children[i]    = new d_node();
				children[i]->inputRice(rc,depth+1);
			}
		}
	}
	void d_node::inputRice4(d_node* parent,rangeDecoder& rd){
		this->parent = parent;
		int count = 0;
		vector<uint> freq(2);
		vector<uint> cumFreq(3);
		int total =  (int)parent->nc.size();
		freq[0] = (int)nc.size();
		freq[1] = (int)parent->nc.size() - freq[0];
		makeCumFreq_dec(freq,cumFreq);
		for(int i = 0; count < (int)nc.size(); i++){
			if(rd.getCharacter(freq,cumFreq,2) == 0){
				nc[count] = parent->nc[i];
				count++;
			}
		}
		if(children.size() != 0){
			int count = 0;
			int n0    = (int)children.size();
			freq[0] = (int)children.size();
			freq[1] = 0x100 - freq[0];
			makeCumFreq_dec(freq,cumFreq);
			for(int i = 0; i < 0x100 && count < (int)children.size(); i++){
				if(rd.getCharacterShift(freq,cumFreq,2,8) == 0){
					children[count]->ch = (BYTE)i;
					children[count]->inputRice4(this,rd);
					count++;
				}
			}
		}
	}
	d_node* d_node::searchPath(BYTE* buf, int p, int historyLimit){
		if(p >= max(historyLimit,0)){
			BYTE cur = buf[p];
			//binary search

			int left = 0;
			int right = (int)children.size();
			int center;
			while(left < right){
				center = (left + right) / 2;
				if(children[center]->ch < cur){
					left = center + 1;
				}
				else if(children[center]->ch > cur){
					right = center;
				}
				else{
					//t->nc[k] == cur
					break;
				}
			}
			if(left != right){
				return children[center]->searchPath(buf,p-1,historyLimit);
			}
		}
		//p < 0 or not found
		return this;
	}
	void d_node::set_ncSkip2(){
		ncSkip.resize(size);
		if(depth == 0){
			for(int i = 0; i < (int)children.size(); i++){
				ncSkip[i] = children[i];
			}
		}
		else{
			int count = 0;
			for(int i = 0; count < (int)nc.size(); i++){
				if(parent->nc[i] == nc[count]){
					d_node* t = parent->ncSkip[i];
					if(t->depth == depth){
						//find the children whose has ch
						int left = 0;
						int right = (int)t->children.size();
						int center;
						while(left < right){
							center = (left + right) / 2;
							if(t->children[center]->ch < ch){
								left = center + 1;
							}
							else if(t->children[center]->ch > ch){
								right = center;
							}
							else{
								//t->nc[k] == cur
								break;
							}
						}
						if(left != right){
							t = t->children[center];
						}
					}
					ncSkip[count] = t;
					count++;
				}
			}
		}
		for(int i = 0; i < (int)children.size(); i++){
			children[i]->set_ncSkip2();
		}
	}
	int d_node::debugSize(){
		int i_size = 0;
		for(int i = 0; i < (int)children.size(); i++){
			i_size += children[i]->debugSize();
		}
		return i_size + 1;
	}
	int d_node::allocateSize(){
		int i_size = 0;
		for(int i = 0; i < (int)children.size(); i++){
			i_size += children[i]->allocateSize();
		}
		i_size += (int)nc.size() + (int)(cumFreq.size() + ncSkip.size() + 7) * sizeof(int);
		//i_size += 7 * sizeof(int);
		return i_size;
	}
	void d_node::freeChildren(){
		for(int i = 0; i < (int)children.size(); i++){
			children[i]->freeChildren();
		}
		children.clear();
	}
	void d_node::freeAll(){
		for(int i = 0; i < (int)children.size(); i++){
			children[i]->freeAll();
			delete children[i];
		}
	}
}