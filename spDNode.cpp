#include"sp.hpp"

namespace sp{
	void d_node::inputRice(riceDecode& rc,int d){
		depth = d;
		No = totalNumber++;
		size = rc.unsigneddecode(RICE_MASK)+1;
		nc.resize(size);

		if(rc.getbit()){
			cumFreq.resize(size + 1);
			if(size == 1) cumFreq[0] = 1;
			else for(int i = 0; i < size; i++){
				int k = rc.unsigneddecode(2) + 1;
				if(k > 2){
					int k1 = k>>1;
					k = rc.unsigneddecode_b(k1) << k1 + (k & 1);
				}
				cumFreq[i] = k;
			}
			makeCumFreq_dec(cumFreq,R_SHIFT);
		}
		if(rc.getbit()){
			int childrenCount = rc.unsigneddecode(RICE_MASK)+1;
			children.resize(childrenCount);
			for(int i = 0; i < childrenCount; i++){
				children[i] = new d_node();
				children[i]->inputRice(rc,depth+1);
			}
		}
	}
	void d_node::inputRice4(d_node* parent,rangeDecoder& rd){
		this->parent = parent;
		int count = 0;
		vector<uint> freq(2), cumFreq(3);
		int total = (int)parent->nc.size(), l=(int)nc.size();
		freq[0] = l;
		freq[1] = total - l;
		makeCumFreq_dec(freq,cumFreq);
		for(int i = 0; count < l; i++)
			if(rd.getCharacter(freq,cumFreq,2) == 0)
				nc[count++] = parent->nc[i];
		if(l=children.size()){
			int count = 0;
			freq[0] = l;
			freq[1] = 256 - l;
			makeCumFreq_dec(freq,cumFreq);
			for(int i = 0; i < 256 && count < l; i++)
				if(rd.getCharacterShift(freq,cumFreq,2,8) == 0){
					children[count]->ch = (BYTE)i;
					children[count]->inputRice4(this,rd);
					count++;
				}
		}
	}
	d_node* d_node::searchPath(BYTE* buf, int p, int historyLimit){
		if(p >= max(historyLimit,0)){
			BYTE cur = buf[p];
			//binary search
			for(int l = 0, r = (int)children.size(), m;l < r;){
				if(children[m = l+r>>1]->ch == cur)
					return children[m]->searchPath(buf,p-1,historyLimit);
				if(children[m]->ch < cur) l = m + 1;
				else r = m;
			}
		}
		return this;//p < 0 or not found
	}
	void d_node::set_ncSkip2(){
		int i = size;
		ncSkip.resize(i);
		if(depth)// 子nodeの場合、親nodeのncSkipを基に設定
			for(int p=parent->nc.size();i;){
				d_node* t = this; // 既定値=自身
				// 親nodeで対応する文字の位置を探す
				for(int c=nc[--i];p;)
					if(parent->nc[--p] == c){
						t = parent->ncSkip[p];
						// tが同じ深度の場合、その子nodeを探す
						if(t->depth == depth)
							// tの子nodeから、ch==thisのchのものを探す
							for(int l = 0, r = (int)t->children.size(), m;l < r;){
								if(t->children[m=l+r>>1]->ch == ch){
									t = t->children[m];goto next;
								}
								if(t->children[m]->ch < ch) l = m + 1;
								else r = m;
							}
						break;
					}
				next:ncSkip[i] = t;
		}else// 根の場合、ncSkipは直接子nodeを指す
			for(;i;){
				ncSkip[--i] = this; // 既定値=自身
				int r = children.size(), c=nc[i];
				// 対応する子nodeを探す
				if(r>3)for(int l = 0, m;l < r;){
					if(children[m=l+r>>1]->ch == c){
						ncSkip[i] = children[m];break;
					}
					if(children[m]->ch < c) l = m + 1;
					else r = m;
				}else for(;r;)
					if(children[--r]->ch == c){
						ncSkip[i] = children[r];break;
					}
			}
		for(int i=(int)children.size();i;)children[--i]->set_ncSkip2();
	}
	int d_node::debugSize(){
		int s = 1;
		for(int i = (int)children.size();i;) s += children[--i]->debugSize();
		return s;
	}
	int d_node::allocateSize(){
		int s = (int)nc.size() + (int)(cumFreq.size() + ncSkip.size() + 7) * sizeof(int);
		for(int i = (int)children.size();i;) s += children[--i]->allocateSize();
		//s += 7 * sizeof(int);
		return s;
	}
	void d_node::freeChildren(){
		for(int i = (int)children.size();i;) children[--i]->freeChildren();
		children.clear();
	}
	void d_node::freeAll(){
		for(int i = (int)children.size();i;delete children[i])
			children[--i]->freeAll();
	}
}