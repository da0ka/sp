/*
	sp
	Daisuke Okanohara (VZV05226@nifty.com)
*/
#include"sp.hpp"

namespace sp{

	int m_number;
	node::node(int* i_freq,int d,node* par,BYTE c)
			:totalFreq(0),oriTotalFreq(0),used(false),
			 mearged(false),kl_div(0.f),toBit(0),initialized(false),
			 meargedNumber(-1),depth(d),parent(par),ch(c){
		size = 0;
		int min = INT_MAX;
		for(int i = 0; i < 0x100; i++){
			if(i_freq[i] != 0){
				size++;
				oriTotalFreq += i_freq[i];
				if(i_freq[i] < min){
					min = i_freq[i];
				}
			}
		}
		nc.resize(size);
		freq.resize(size);

		int t = 0;
		while(min){
			min >>= 1;
			t++;
		}
		//fprintf(stderr,"%d\n",k);

		size = 0;
		for(int i = 0; i < 0x100; i++){
			if(i_freq[i] != 0){
				nc[size] = i;
				//freq[size] = (i_freq[i] >> k);
				//freq[size] = i_freq[i];

				if(i_freq[i] > 1){
					int k = 0;
					int t = i_freq[i];
					while(t){
						t >>= 1;
						k++;
					}
					k = (k / 2) +  (k & 1);
					i_freq[i] >>= k;
					i_freq[i] <<= k;
				}
				freq[size] = i_freq[i];
				totalFreq += i_freq[i];
				size++;
			}
		}
	}
	float node::kl_divergence(node* par){
		// sigma(chi * log (chi / par))
		int j = 0;
		float kl = 0.f;
		for(int i = 0; i < (int)freq.size(); i++){
			BYTE c = nc[i];
			//search parent
			while(par->nc[j] != c && j < (int)par->freq.size()) j++;
			if(j == par->freq.size()){
				throw "kl_dirvergence error";
			}
			float f_c = (float)freq[i] / totalFreq;
			float f_p = (float)par->freq[j] / par->totalFreq;
			kl += f_c * log(f_c / f_p);
		}
		kl /= log(2.f);
		return oriTotalFreq * kl;
	}
	float node::kl_divergence2(node* par){
		// sigma(chi * log (chi / par))
		int j = 0;
		float kl = 0.f;
		for(int i = 0; i < (int)freq.size(); i++){
			BYTE c = nc[i];
			//search parent
			while(par->nc[j] != c && j < (int)par->freq.size()) j++;
			if(j == par->freq.size()){
				throw "kl_dirvergence error";
			}
			float f_c = (float)freq[i] / totalFreq;
			float f_p = (float)par->freq[j] / par->totalFreq;
			//printf("%f %f\n",f_c,f_p);
			kl += f_c * log(f_c / f_p);
		}
		kl /= log(2.f);
		return oriTotalFreq * kl;
	}
	int riceUnsignedN(unsigned int n, int mask){
		if(n < 0){
			throw "error";
		}
		int middle = n >> mask;
		return 1 + middle + mask;
	}
	int node::checkBitN(node* parent){
		int bitcount = 0;
		bitcount += riceUnsignedN((int)nc.size(),RICE_MASK);
	//	int p = 0;
	//	int t = totalFreq;
	/*
		int t = 0;
		for(vector<uint>::iterator i = freq.begin(); i != freq.end(); i++){
			t += *i;
		}
		while(t){
			t >>= 1;
			p++;
		}
		p /= 2;

		if(nc.size() != 1){
			bitcount += riceUnsignedN(p,RICE_MASK);
		}
	*/

		int count = 0;
		int dif   = 0;

		//put nc
		count = 0;

		int n0 = (int)nc.size();
		int total = (int)parent->nc.size();
		float ltotal = (total != 0) ? log((float)total) : 0.f;
		float ln0    = (n0    != 0) ? log((float)n0) : 0.f;
		float ln1    = (total - n0 != 0) ? log((float)(total - n0)) : 0.f;
		bitcount += (int)((total * ltotal - n0 * ln0 - (total-n0) * ln1) / log(2.f));

		for(int i = 0; i < (int)parent->nc.size() && count < (int)nc.size(); i++){
			if(parent->nc[i] == nc[count]){

				if(nc.size() != 1){
					//bitcount += riceUnsignedN(freq[count] - 1,p);

					int t = freq[count] - 1;
					int k = 0;
					while(t){
						t >>= 1;
						k++;
					}
					//fprintf(stderr,"%d\n",k);
					bitcount += riceUnsignedN(k,2);
					if(k != 0){
						k = (k / 2) - (k & 1);
						//int k2 = (k / 2) + (k & 1);
						bitcount += k;//rc.unsignedcode_b(t >> k2,k);
					}
				}
				count++;
			}
		}
		count = 0;
		dif = 0;

		bitcount++; //children count

		return bitcount;
	}
	int node::checkBitN3(){
		int bitcount = 0;
		int p = 0;
		int t = totalFreq;
		while(t){
			t >>= 1;
			p++;
		}
		p /= 2;
		if(nc.size() != 1){
			bitcount += riceUnsignedN(p,RICE_MASK);
		}
		for(int i = 0; i < (int)nc.size(); i++){
			bitcount += riceUnsignedN(freq[i] - 1,p);
		}
		return bitcount;
	}
	int node::checkBitN2(){
		int n0 = (int)children.size();
		int total = 0x100;
		float ltotal = (total != 0) ? log((float)total) : 0.f;
		float ln0    = (n0    != 0) ? log((float)n0) : 0.f;
		float ln1    = (total - n0 != 0) ? log((float)(total - n0)) : 0.f;

		int bitcount = riceUnsignedN((int)children.size(),RICE_MASK);
		bitcount += (int)((total * ltotal - n0 * ln0 - (total-n0) * ln1) / log(2.f));

		return bitcount;
	}
	int node::getSize(){
		int i_size = 0;
		for(vector<node*>::iterator i = children.begin(); i != children.end(); i++){
			i_size += (*i)->getSize();
		}
		return i_size + 1;
	}
	void node::free(){
		for(vector<node*>::iterator i = children.begin(); i != children.end(); i++){
			(*i)->free();
			delete *i;
		}
	}
	int node::getNonMeargeSize(){
		int i_size = 0;
		for(vector<node*>::iterator i = children.begin(); i != children.end(); i++){
			i_size += (*i)->getNonMeargeSize();
		}
		if(mearged == 0) return i_size + 1;
		else return i_size;
	}
	int node::getUsedSize(){
		int i_size = 0;
		for(vector<node*>::iterator i = children.begin(); i != children.end(); i++){
			i_size += (*i)->getUsedSize();
		}
		if(used == true) return i_size + 1;
		else return i_size;
	}
	node* node::searchPath(vector<BYTE>& buf, int p, int historyLimit){
		if(p >= historyLimit){
			BYTE cur = buf[p];
			//liner search
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
	void node::outputRice(riceEncode& rc,bool m){
		rc.unsignedcode((int)nc.size(),RICE_MASK);

		if(used == true){
			rc.putbit(1);
			if(nc.size() != 1){
				for(int i = 0; i < (int)nc.size(); i++){
					int t = oriFreq[i];
					int k = 0;
					while(t){
						t >>= 1;
						k++;
					}
					rc.unsignedcode(k-1,2);
					if(k > 2){
						int k1 = (k / 2);
						int k2 = (k / 2) + (k & 1);
						rc.unsignedcode_b(oriFreq[i] >> k2,k1);
					}
				}
			}
		}
		else{
			rc.putbit(0);
			/*
			if(mearged == 1){
				rc.putbit(1);
				if(meargedNode->meargedNumber == -1){
					rc.putbit(1);
					meargedNode->outputRice(parent,rc,true);
				}
				else{
					rc.putbit(0);
					rc.unsignedcode(meargedNode->meargedNumber,RICE_MASK);
				}
			}
			else{
				rc.putbit(0);
			}
			*/
		}
		//fprintf(stderr,"%d\n",children.size());

		if(m == true){
			meargedNumber = m_number;
			m_number++;
			return;
		}
		if(children.size() == 0){
			rc.putbit(0);
		}
		else{
			rc.putbit(1);
			rc.unsignedcode((int)children.size(),RICE_MASK); //about 10bit
			for(int i = 0; i < (int)children.size(); i++){
				children[i]->outputRice(rc,false);
			}
		}
	}
	void node::outputRice4(node* parent,rangeEncoder& ra, bool m){
		//fprintf(stderr,"%d %d\n",parent->nc.size(),nc.size());
		int count = 0;
		uint n0    = (int)nc.size();
		uint total = (int)parent->nc.size();
		for(int i = 0; count < (int)nc.size(); i++){
			if(parent->nc[i] == nc[count]){
				ra.encode(0,n0,total);
				count++;
			}
			else{
				ra.encode(n0,total-n0,total);
			}
		}
		if(m == true){
			meargedNumber = m_number;
			m_number++;
			return;
		}
		if(children.size() != 0){
			int count = 0;
			int n0    = (int)children.size();
			for(int i = 0; i < 0x100 && count < (int)children.size(); i++){
				if(children[count]->ch == i){
					ra.encodeshift(0,n0,8);
					children[count]->outputRice4(this,ra,false);
					count++;
				}
				else{
					ra.encodeshift(n0,0x100-n0,8);
				}
			}
		}
	}
	int checkLog2(int n){
		if(n < 0){
			printf("%d\n",n);
			throw "error";
		}
		int t = 0;
		while(n > 0){
			n >>= 1;
			t++;
		}
		return t;
	}
	void node::mearge(){
		vector<node*> c;
		for(vector<node*>::iterator i = children.begin(); i != children.end(); i++){
			if((*i)->children.size() == 0){
				c.push_back(*i);
			}
			else{
				(*i)->mearge();
			}
		}
		int beforeSize = (int)c.size();

		//greedy stratedy
		int* cf = new int[0x100];
		vector<node*> new_c;
		for(;;){
			if(c.size() < 2) break;
			int mn = 0;
			for(int i = 0; i < (int)c.size(); i++){
				for(int j = i+1; j < (int)c.size(); j++){
					memset(cf,0,sizeof(int) * 0x100);
					node* a = c[i];
					node* b = c[j];

					while(a->mearged == true){
						a = a->meargedNode;
					}
					while(b->mearged == true){
						b = b->meargedNode;
					}
					if(a == b) continue;

					for(int k = 0; k < (int)a->freq.size(); k++){
						cf[a->nc[k]] += a->freq[k];
					}
					for(int k = 0; k < (int)b->freq.size(); k++){
						cf[b->nc[k]] += b->freq[k];
					}
					node* t = new node(cf,a->depth,a->parent,a->ch);

					float gain = a->kl_divergence2(t);
						  gain += b->kl_divergence2(t);

					t->toBit = t->checkBitN3();
					int   bitCount = a->toBit + b->toBit - t->toBit + (int)a->freq.size() + (int)b->freq.size() - (int)t->freq.size() + 1;
					//check the increasing cost and benefit

					if(gain < bitCount){

						mn ++;
						//mearge
						a->mearged = true;
						b->mearged = true;
						a->meargedNode = t;
						b->meargedNode = t;

						t->toBit = min(a->toBit,b->toBit);
						new_c.push_back(t);
					}
					else{
						delete t;
					}
				}
			}
			for(int i = 0; i < (int)c.size(); i++){
				if(c[i]->mearged == false){
					new_c.push_back(c[i]);
				}
			}
			c = new_c;
			new_c.clear();

			if(mn == 0) break;

		}
		if(cf) delete[] cf;
	}
	void node::set_ncSkip2(){
		if(initialized == true) return;
		initialized = true;
		cumFreq.resize(nc.size());
		oriFreq.resize(nc.size());
		copy(freq.begin(),freq.end(),oriFreq.begin());
		normalize(freq,R_SHIFT);
		makeCumFreq(freq,cumFreq);
		ncSkip.resize(nc.size());

		if(depth == 0){
			for(int i = 0; i < (int)children.size(); i++){
				ncSkip[i] = children[i];
			}
		}
		else{
			int count = 0;
			for(int i = 0; count < (int)nc.size(); i++){
				if(parent->nc[i] == nc[count]){
					node* t = parent->ncSkip[i];
					if(t->depth == depth){
						//find the children whose has ch.
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
}