/*
	sp
	Daisuke Okanohara (VZV05226@nifty.com)
*/
#include"sp.hpp"

namespace sp{
	node* eModel::makeModel(){
		//---: make tree-----------
		memset(freq,0,sizeof(int) * 0x100);
		memset(freq2,0,sizeof(int) * 0x100 * 0x100);

		freq2[buf[0] << 8]++; //freq2[(buf[0] << 8) + buf[-1]]++;
		for(int i = 1 ; i < bufSize; i++){
			freq[buf[i]]++;
			freq2[(buf[i] << 8) + buf[i-1]]++;
		}
		int max = freq2[0];
		for(int i = 1; i < 0x10000; i++){
			if(freq2[i] > max) max = freq2[i];
			freq2[i] += freq2[i-1];
		}
		for(int i = bufSize - 1; i >= 1; i--){
			pos[--freq2[(buf[i] << 8) + buf[i-1]]] = i;
		}
		pos[--freq2[buf[0] << 8]] = 0;

		work = new int[max];
		if(work == 0) throw "work alocate error";
		node* root = new node(freq,0,NULL,0);
		if(root == 0) throw "root allocate error";
		makeTree(root,0,bufSize,0);

		//printf("tree size:%d\n",root->getSize());

		if(work) delete[] work;
		return root;
	}
void eModel::makeTree(node* parent, int start, int last, int depth){
	/*
	if(depth == MAXDEPTH){
		return;
	}
	*/

	if(pos[start] - depth < 0){
		start++;
	}
	//naive check
	{
		int cur = buf[pos[start] + 1];
		int i = start + 1;
		for(; i < last; i++){
			if(cur != buf[pos[i] + 1]) break;
		}
		if(i == last){
			//everything is same. we don't have to check more.
			return;
		}
	}
	if(depth >= 2){
		//We already sort depth 0,1.
		if(last - start < 32){
			//Insertion sort
			for(int i = start + 1; i < last; i++){
				int x = pos[i];
				int j;
				for(j = i - 1; j >= start && buf[pos[j] - depth] > buf[x - depth]; j--){
					pos[j + 1] = pos[j];
				}
				pos[j + 1] = x;
			}
		}
		else
		{
			//Bucket sort
			memset(freq,0,sizeof(int) * 0x100);
			for(int i = start; i < last; i++){
				freq[buf[pos[i] - depth]]++;
			}
			for(int i = 1; i < 0x100; i++){
				freq[i] += freq[i-1];
			}
			for(int i = last - 1; i >= start; i--){
				work[--freq[buf[pos[i] - depth]]] = pos[i];
			}
			memcpy(pos + start,work,(last - start) * sizeof(int));
		}
	}
	//Check whether likelihood is increased.

	vector<node*> children;
	vector<BYTE>  children_ch;
	vector< pair<int,int> > range;

	int bitCount = 0;
	float gain   = 0.f;
	for(int i = start; i < last; ){
		memset(freq,0,sizeof(int) * 0x100);

		if(pos[i] - depth < 0){
			fprintf(stderr,"%d %d\n",start,i);
			throw "pos[i] - depth is minus";
			i++;

			continue;
		}
		BYTE cur = buf[pos[i] - depth];
		if(pos[i] < bufSize - 1){
			freq[buf[pos[i] + 1]]++;	//predict
		}
		//check how many character is same
		int j = 1;

		if((pos[i] - depth) != bufSize){
			for(; i+j < last; j++){
				if(pos[i + j] - depth < 0){
					throw "error2";
				}
				if(cur != buf[pos[i + j] - depth]){
					break;
				}
				if(pos[i + j] + 1 < bufSize){
					freq[buf[pos[i + j] + 1]]++;
				}
			}
		}
		else{
			i++;
			continue;
		}
		//i , i+1, ..., i+j-1 is conserned
		node* st = new node(freq,depth+1,parent,cur);
		if(st == 0){
			throw "st allocate error";
		}
		if(st->totalFreq == 0){
			delete st;
			i += j;
			continue;
		}
		children.push_back(st);
		range.push_back(pair<int,int>(i,i+j));
		i += j;

		float kl_div = st->kl_divergence(parent);
		gain += kl_div;
		st->kl_div = kl_div;

		int toBit = st->checkBitN(parent);
		bitCount += toBit;
		//st->toBit = st->checkBitN3();
	}
	bitCount += parent->checkBitN2();

	if(gain < bitCount){
		for(vector<node*>::iterator i = children.begin(); i != children.end(); i++){
			delete *i;
		}
		return;
	}
	//register
	parent->children = children;

	//recursion
	for(int i = 0; i < (int)children.size(); i++){
		makeTree(children[i],range[i].first,range[i].second,depth + 1);
	}
}
void eModel::compress(FILE* infp,FILE* outfp){
	int remainSize = fileSize;

	printf(" INTERVAL:%d\n",interval);
	fwrite(&fileSize,sizeof(int),1,outfp);
	fwrite(&interval,sizeof(int),1,outfp);
	rangeEncoder e(outfp);
	vector<int> out_pos((fileSize / interval) + 1); //Position at Compressed Data.
	vector<BYTE> history(MAXDEPTH);

	out_pos[0] = ftell(outfp);
	int inPos = 1;

	vector<int> treePos;


	for(int i = 0; i < 0x100; i++){freq[i] = 1;}
	node base(freq,-1,NULL,0);

	base.oriFreq.resize(0x100);
	base.children.resize(0x100);

	for(;;){
		int shouldReadSize = (remainSize > MAXBUF)? MAXBUF : remainSize;
		int readSize = (int)fread(buf,sizeof(BYTE),shouldReadSize,infp);
		if(readSize != shouldReadSize){
			fprintf(stderr,"readSize:%d\n",readSize);
			throw "read miss\n";
		}
		if(readSize == 0) break;
		remainSize -= readSize;
		printf("processing %10dB ",fileSize - remainSize);

		bufSize = readSize;

		printf("make model");
		node* root = makeModel();	//add root to branch
		printf("(state:%d) ",root->getSize());

		root->set_ncSkip2();

		e.setup();

		int step = 0;

		node* t = root; //start
		printf("coding ");
		for(int i = 0; i < readSize; i++){
			//node* t = root->searchPath(buf,i-1,0);

			node* t_m = t;
			//if(t->mearged == true) t_m = t->meargedNode;
			//else t_m = t;
			t_m->used = true;

			//Binary Search
			int left  = 0;
			int right = (int)t->nc.size();
			//fprintf(stderr,"%d\n",right);
			int p;
			while(left < right){
				p = (left + right) / 2;
				if      (t->nc[p] < buf[i]) left = p + 1;
				else if(t->nc[p] > buf[i])	right = p;
				else break; // t->nc[p] == buf[i]
			}
			if(left == right){
				printf("index:%d\n",i);
				throw "cannot found nc";
			}
			int p_m;
			left = 0;
			right = (int)t_m->nc.size();
			while(left < right){
				p_m = (left + right) / 2;
				if      (t_m->nc[p_m] < buf[i]) left = p_m + 1;
				else if(t_m->nc[p_m] > buf[i])	right = p_m;
				else break; // t->nc[p] == buf[i]
			}
			if(left == right){
				printf("index:%d\n",i);
				throw "cannot found nc";
			}
			if(p_m == t_m->nc.size()){
				//printf("%d %d %d %d %d %d %d\n",i,p,t->size,buf[i],t->nc[0],t->depth,buf[i-1]);
				throw "cannot found nc";
			}
			//e.encodeshift(t_m->cumFreq[p_m],t_m->cumFreq[p_m+1] - t_m->cumFreq[p_m],R_SHIFT);
			e.encodeshift(t->cumFreq[p],t->freq[p],R_SHIFT);

			step++;
			if(step == interval){
				step = 0;
				e.flush();
				e.setup();
				out_pos[inPos] = ftell(outfp);
				inPos++;
				t = root;
				continue;
			}
			t = t->ncSkip[p];
		}
		e.flush();
		treePos.push_back(ftell(outfp));
		riceEncode rc(RICE_MASK,outfp);
		root->outputRice(rc,false);
		rc.flush();
		rangeEncoder ra(outfp);
		root->outputRice4(&base,ra,false);
		ra.flush();
		//printf("treeSize:%d\n",(*i)->getSize());
		m_number = 0;
		int outHeaderSize = ftell(outfp);
		root->free();
		delete root;
		printf("done \n");
		out_pos[--inPos] = ftell(outfp);
		inPos++;
	}
	{
		/*
			contents of footer

			XXX <- nowPos
			XXX
			XXX

			nowPos
		*/
		int nowPos = ftell(outfp);
		{
			riceEncode rc(RICE_MASK,outfp);
			rc.unsignedcode(out_pos[0],RICE_MASK);
			int average = (out_pos[fileSize / interval] - out_pos[0]) / (fileSize / interval + 1);
			//float favg  = (float)(out_pos[fileSize / interval] - out_pos[0]) / (fileSize / interval + 1);
			rc.unsignedcode(average,RICE_MASK);
			for(int i = 1; i < (fileSize / interval) + 1; i++){
				int tmp = out_pos[i] - out_pos[i-1];
				rc.code(tmp-average,2);
				//fprintf(stderr,"%d %d %d %d\n",tmp-average,(int)(i * favg) + out_pos[0],out_pos[i],out_pos[i] - (int)(i * favg) - out_pos[0]);
			}
			rc.flush();
		}
		for(vector<int>::iterator i = treePos.begin(); i != treePos.end(); i++){
			int tmp = *i;
			fwrite(&tmp,sizeof(int),1,outfp);
		}
		fwrite(&nowPos,sizeof(int),1,outfp);
	}
//	if(history) delete[] history;

	int outBufSize = ftell(outfp);
	printf("ratio:%.3f\n",(float)outBufSize / fileSize * 100);
}
}