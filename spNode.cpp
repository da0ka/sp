#include "sp.hpp"
namespace sp{
	int m_number;
	node::node(int* i_freq,int d,node* par,BYTE c)
			:totalFreq(0),oriTotalFreq(0),used(false),
			 mearged(false),kl_div(0.f),toBit(0),initialized(false),
			 meargedNumber(-1),depth(d),parent(par),ch(c){
		size = 0;
		int min = INT_MAX;
		for(int i = 256 ,f;i;)
			if(f=i_freq[--i]){
				size++;
				oriTotalFreq += f;
				if(f < min) min = f;
			}
		nc.resize(size);
		freq.resize(size);

//		int t = 0;for(;min;t++) min >>= 1;fprintf(stderr,"%d\n",t);
		for(int i = d=0, f; i < 256; i++)
			if(f=i_freq[i]){
				nc[d] = i;
				//freq[d] = f >> t;

				if(f > 1){
					int k = 0;
					for(;f>>++k;);
					f >>= k = (k>>1) + (k&1);
					i_freq[i]=f<<=k;
				}
				totalFreq += freq[d++] = f;
			}
	}
	float node::kl_divergence(node* par){
		// sigma(chi * log (chi / par))
		float kl = 0.f;
		for(int i = 0, j = 0, l=(int)freq.size(), m=(int)par->freq.size(); i < l; i++){
			BYTE c = nc[i];
			//search parent
			while(par->nc[j] != c && j < m) j++;
			if(j == m) throw"kl_dirvergence error";
			float f_c = (float)freq[i] / totalFreq;
			float f_p = (float)par->freq[j] / par->totalFreq;
			kl += f_c * log(f_c / f_p);
		}
		kl /= log(2.f);
		return oriTotalFreq * kl;
	}
	float node::kl_divergence2(node* par){
		// sigma(chi * log (chi / par))
		float kl = 0.f;
		for(int i = 0, j = 0, l=(int)freq.size(), m=(int)par->freq.size(); i < l; i++){
			BYTE c = nc[i];
			//search parent
			while(par->nc[j] != c && j < m) j++;
			if(j == m) throw"kl_dirvergence error";
			float f_c = (float)freq[i] / totalFreq;
			float f_p = (float)par->freq[j] / par->totalFreq;
			kl += f_c * log(f_c / f_p);
		}
		kl /= log(2.f);
		return oriTotalFreq * kl;
	}
	int riceUnsignedN(unsigned int n, int mask){
		if(n < 0) throw"error";
		return(n >> mask) + mask+1;
	}
	int node::checkBitN(node* parent){
		int n0 = (int)nc.size(), total = (int)parent->nc.size();
		int bitcount = riceUnsignedN(n0,RICE_MASK);
	//	int p = 0, t = totalFreq;
	/*
		int t = 0;
		for(vector<uint>::iterator i = freq.begin(); i != freq.end(); i++) t += *i;
		for(;t;p++) t >>= 1;
		p /= 2;
		if(n0 != 1) bitcount += riceUnsignedN(p,RICE_MASK);
	*/
		int count = 0, dif = 0;
		//put nc
		float ltotal = total ? log((float)total) : 0.f;
		float ln0 = n0 ? log((float)n0) : 0.f;
		float ln1 = total > n0 ? log((float)(total-n0)) : 0.f;
		bitcount += (int)((total * ltotal-n0 * ln0 - (total-n0) * ln1) / log(2.f));

		for(int i = 0; i < total && count < n0; i++)
			if(parent->nc[i] == nc[count]){
				if(n0 != 1){
					//bitcount += riceUnsignedN(freq[count] - 1,p);
					int t = freq[count]-1, k = 0;
					for(;t;k++) t >>= 1;
					bitcount += riceUnsignedN(k,2);
					if(k){
						bitcount += k = (k>>1) - (k & 1);
						//rc.unsignedcode_b(t >>(k>>1) + (k & 1),k);
					}
				}
				count++;
			}
		return++bitcount;//children count
	}
	int node::checkBitN3(){
		int bitcount = 0, p = 0, t = totalFreq;
		for(;t;p++) t >>= 1;
		p>>=1;
		if(nc.size() != 1) bitcount += riceUnsignedN(p,RICE_MASK);
		for(int i = (int)nc.size();i;) bitcount += riceUnsignedN(freq[--i] - 1,p);
		return bitcount;
	}
	int node::checkBitN2(){
		int n0 = (int)children.size(), total = 256;
		float ltotal = log((float)total);
		float ln0 = n0 ? log((float)n0) : 0.f;
		float ln1 = total > n0 ? log((float)(total-n0)) : 0.f;
		int bitcount = riceUnsignedN((int)children.size(),RICE_MASK);

		return bitcount+(int)((total * ltotal-n0 * ln0 - (total-n0) * ln1) / log(2.f));
	}
	int node::getSize(){
		int i_size = 1;
		for(vector<node*>::iterator i = children.begin(); i != children.end(); i++)
			i_size += (*i)->getSize();
		return i_size;
	}
	void node::free(){
		for(vector<node*>::iterator i = children.begin(); i != children.end(); i++){
			(*i)->free();
			delete *i;
		}
	}
	int node::getNonMeargeSize(){
		int i_size = mearged == 0;
		for(vector<node*>::iterator i = children.begin(); i != children.end(); i++)
			i_size += (*i)->getNonMeargeSize();
		return i_size;
	}
	int node::getUsedSize(){
		int i_size = used == true;
		for(vector<node*>::iterator i = children.begin(); i != children.end(); i++)
			i_size += (*i)->getUsedSize();
		return i_size;
	}
	node* node::searchPath(BYTE* buf, int p, int historyLimit){
		if(p >= historyLimit){
			BYTE cur = buf[p];
			//binary search
			for(int l = 0, r = (int)children.size(), m;l < r;){
				if(children[m = l+r>>1]->ch == cur) return children[m]->searchPath(buf,p-1,historyLimit);
				if(children[m]->ch < cur) l = m + 1;
				else r = m;
			}
		}
		return this;//p < 0 or not found
	}
	void node::outputRice(riceEncode& rc,bool m){
		int ns=(int)nc.size();
		rc.unsignedcode(ns-1,RICE_MASK);
		rc.putbit(used);
		if(used){
			if(ns!=1)
				for(int i = 0; i < ns; i++){
					int t = oriFreq[i], k = 0;
					for(;t>>++k;);
					rc.unsignedcode(k-1,2);
					if(k > 2){
						int k1 = k>>1;
						rc.unsignedcode_b(t>>k1 + (k & 1),k1);
					}
				}
		}//else{
			/*
			if(mearged == 1){
				rc.putbit(1);
				if(meargedNode->meargedNumber == -1){
					rc.putbit(1);
					meargedNode->outputRice(parent,rc,true);
				}else{
					rc.putbit(0);
					rc.unsignedcode(meargedNode->meargedNumber,RICE_MASK);
				}
			}else rc.putbit(0);
			*/
//		}
		if(m){
			meargedNumber = m_number++;
			return;
		}
		int cs=children.size();
		rc.putbit(cs>0);
		if(cs){
			rc.unsignedcode(cs-1,RICE_MASK); //about 10bit
			for(int i=0;i<cs;i++) children[i]->outputRice(rc,false);
		}
	}
	void node::outputRice4(node* parent,rangeEncoder& ra, bool m){
		uint n0 = (int)nc.size(), total = (int)parent->nc.size();
		int count = 0, s=n0;
		for(int i = 0; count < s; i++)
			if(parent->nc[i] == nc[count]){
				ra.encode(0,n0,total);
				count++;
			}else ra.encode(n0,total-n0,total);
		if(m){
			meargedNumber = m_number++;
			return;
		}
		if(n0=children.size())
			for(int i = count = 0; i < 256 && count < n0; i++)
				if(children[count]->ch == i){
					ra.encodeshift(0,n0,8);
					children[count++]->outputRice4(this,ra,false);
				}else ra.encodeshift(n0,256-n0,8);
	}
	int checkLog2(int n){
		if(n < 0){
			printf("%d\n",n);
			throw"error";
		}
		int t = 0;
		for(;n > 0;t++) n >>= 1;
		return t;
	}
	void node::mearge(){
		vector<node*> c;
		for(vector<node*>::iterator i = children.begin(); i != children.end(); i++)
			if((*i)->children.size()) (*i)->mearge();
			else c.push_back(*i);
		//greedy stratedy
		int *cf = new int[256], cs = (int)c.size();
		vector<node*> new_c;
		for(;cs>1;){
			int mn = 0;
			for(int i = 0; i < cs; i++){
				for(int j = i+1; j < cs; j++){
					node* a = c[i], *b = c[j];

					while(a->mearged) a = a->meargedNode;
					while(b->mearged) b = b->meargedNode;
					if(a == b) continue;
					memset(cf,0,sizeof(int) * 256);
					for(int k = 0, l=(int)a->freq.size(); k < l; k++)
						cf[a->nc[k]] += a->freq[k];
					for(int k = 0, l=(int)b->freq.size(); k < l; k++)
						cf[b->nc[k]] += b->freq[k];
					node* t = new node(cf,a->depth,a->parent,a->ch);
					float gain = a->kl_divergence2(t)+b->kl_divergence2(t);

					t->toBit = t->checkBitN3();
					int bitCount = a->toBit + b->toBit - t->toBit + (int)a->freq.size() + (int)b->freq.size() - (int)t->freq.size() + 1;
					//check the increasing cost and benefit

					if(gain < bitCount){
						mn++;
						//mearge
						a->mearged = b->mearged = true;
						a->meargedNode = b->meargedNode = t;

						t->toBit = min(a->toBit,b->toBit);
						new_c.push_back(t);
					}else delete t;
				}
			}
			for(int i = 0; i < cs; i++)
				if(!c[i]->mearged)
					new_c.push_back(c[i]);
			c = new_c;cs=c.size();
			new_c.clear();
			if(!mn) break;
		}
		if(cf) delete[] cf;
	}
	void node::set_ncSkip2(){
		if(initialized) return;
		initialized = true;
		int i = nc.size();
		cumFreq.resize(i);
		oriFreq.resize(i);
		copy(freq.begin(),freq.end(),oriFreq.begin());
		normalize(freq,R_SHIFT);
		makeCumFreq(freq,cumFreq);
		ncSkip.resize(i);

		if(depth)// 子nodeの場合、親nodeのncSkipを基に設定
			for(int p=parent->nc.size();i;){
				node* t = this; // 既定値=自身
				// 親nodeで対応する文字の位置を探す
				for(int c=nc[--i];p;)
					if(parent->nc[--p] == c){
						t = parent->ncSkip[p];
						// tが同じ深度の場合、その子nodeを探す
						if(t->depth == depth)
							// tの子nodeから、ch==thisのchのものを探す
							for(int l = 0, r = (int)t->children.size(), m;l < r;){
								if(t->children[m=l+r>>1]->ch == ch){
									t = t->children[m];
									goto next;
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
		for(int i = (int)children.size();i;) children[--i]->set_ncSkip2();
	}
}