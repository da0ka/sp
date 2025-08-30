#include "sp.hpp"

namespace sp{
node* eModel::makeModel(){
	// make tree
	memset(freq,0,sizeof(int) * 256);
	int freq2[65536]={0};

	freq2[buf[0]<<8]++; //freq2[(buf[0] << 8) + buf[-1]]++;
	for(int i = 0; ++i < bufSize;freq[buf[i]]++)
		freq2[buf[i]<<8|buf[i-1]]++;
	int max = freq2[0];
	for(int i = 0, c=max;c<bufSize;c=freq2[i]+=c)
		if(freq2[++i] > max) max = freq2[i];
	for(int i = bufSize;--i>0;)
		pos[--freq2[buf[i]<<8|buf[i-1]]] = i;
	freq[buf[pos[--freq2[buf[0]<<8]] = 0]]++;

	work = new int[max];
	if(!work) throw"work alocate error";
	node* root = new node(freq,0,NULL,0);
	if(!root) throw"root allocate error";
	makeTree(root,0,bufSize,0);
	if(work) delete[] work;
	return root;
}
void eModel::makeTree(node* parent, int start, int last, int depth){
	//if(depth == MAXDEPTH) return;
	if(pos[start] < depth) start++;
	//naive check
	int i = start, s = buf[pos[i]+1];
	for(;++i < last && s == buf[pos[i]+1];);
	if(i == last) return;//everything is same. we don't have to check more
	BYTE*B=buf-depth;
	if(depth>1){
		//We already sort depth 0,1
		i = start, s=last - i;
		if(s<32)//Insertion sort
			for(int j=i;++i < last;j=i){
				for(s = pos[i]; j >= start && B[pos[j]] > B[s]; j--)
					pos[j+1] = pos[j];
				pos[j+1] = s;
			}
		else{//Bucket sort
			memset(freq,0,sizeof(int) * 256);
			for(;i < last;) freq[B[pos[i++]]]++;
			for(int *f=freq, c=*f;c<s;) c=*++f+=c;
			for(;--i >= start;) work[--freq[B[pos[i]]]] = pos[i];
			memcpy(pos + start,work,s*sizeof(int));
		}
	}
	//Check whether likelihood is increased
	vector<node*> children;
	vector< pair<int,int> > range;

	int bitCount = 0;
	float gain = 0;
	for(int i = start, p; i < last;){
		if((p=pos[i]) < depth){
			fprintf(stderr,"%d %d\n",start,i);
			throw"pos[i] - depth is minus";
			i++;
			continue;
		}
		if(p - depth == bufSize){i++;continue;}
		memset(freq,0,sizeof(int) * 256);
		BYTE cur = B[p];
		//check how many character is same
		int j = i;
		for(freq[buf[p+1]]=p+1 < bufSize; ++i < last;){
			if((p=pos[i]) < depth) throw"error2";
			if(cur != B[p]) break;
			if(++p < bufSize) freq[buf[p]]++;
		}
		//j , j+1, ..., i-1 is concerned
		node* st = new node(freq,depth+1,parent,cur);
		if(!st) throw"st allocate error";
		if(st->totalFreq == 0){
			delete st;
			continue;
		}
		children.push_back(st);
		range.push_back(pair<int,int>(j,i));
		gain += st->kl_div = st->kl_divergence(parent);
		bitCount += st->checkBitN(parent);
		//st->toBit = st->checkBitN3();
	}
	bitCount += parent->checkBitN2();
	if(gain < bitCount){
		for(vector<node*>::iterator b=children.begin(), e=children.end();b!=e;)
			delete *--e;
		return;
	}
	//register
	parent->children = children;
	//recursion
	for(int i = (int)children.size();i--;)
		makeTree(children[i],range[i].first,range[i].second,depth+1);
}
void eModel::compress(FILE* infp,FILE* outfp){
	int remainSize = fileSize, inPos = 0;

	printf(" INTERVAL:%d\n",interval);
	fwrite(&fileSize,sizeof(int),1,outfp);
	fwrite(&interval,sizeof(int),1,outfp);
	rangeEncoder e(outfp);
	vector<int> out_pos(fileSize/interval+1); //Position at Compressed Data
	vector<int> treePos;
//	vector<BYTE> history(MAXDEPTH);

	out_pos[0] = ftell(outfp);
	for(int i = 256;i;)freq[--i] = 1;
	node base(freq,-1,NULL,0);

	base.oriFreq.resize(256);
	base.children.resize(256);
	for(;;){
		int shouldReadSize = remainSize > MAXBUF? MAXBUF : remainSize;
		int readSize = (int)fread(buf,sizeof(BYTE),shouldReadSize,infp);
		if(readSize != shouldReadSize){
			fprintf(stderr,"readSize:%d\n",readSize);
			throw"read miss\n";
		}
		if(!readSize) break;
		remainSize -= bufSize = readSize;
		printf("processing %10dB make model",fileSize - remainSize);
		node* root = makeModel();	//branch
		printf("(state:%d) ",root->getSize());

		root->set_ncSkip2();
		e.setup();

		int step = 0;
		node* t = root; //start
		puts("coding");
		for(int i = 0; i < readSize; i++){
			//node* t = root->searchPath(buf,i-1,0);
			node* t_m = t;
			//if(t->mearged == true) t_m = t->meargedNode;else t_m = t;
			t_m->used = true;

			//Binary Search
			int c=buf[i], left = 0, right = (int)t->nc.size(), p=0;
			if(right>1){
				while(left < right && t->nc[p = left+right>>1] != c)
					t->nc[p]<c? left = p+1: right = p;
				if(left == right){
					printf("index:%d\n",i);
					throw"cannot found nc";
				}
				e.encodeshift(t->cumFreq[p],t->freq[p],R_SHIFT);
			}
			if(++step == interval){
				step = 0;
				e.flush();e.setup();
				out_pos[++inPos] = ftell(outfp);
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
		m_number = 0;
		root->free();
		delete root;
		puts("done");
		out_pos[inPos] = ftell(outfp);
	}
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
		int x=-1u>>1, v = (out_pos[0] - out_pos[inPos])/~inPos, min=x, avg=v;
		//converted to sequence of differences
		for(int i=inPos,n;i;--i){
			n=out_pos[i]-=out_pos[i-1];
			if(n<min)min=n;
		}
		//find best avg
		for(;min<v--;){
			int i=0,c=0;
			for(;i<inPos;){
				int n=out_pos[++i]-v,b=0;
				if(n<0)n=~n;
				for(;n>>++b;);
				c+=(b>>2)+(b>1?b+3:5);
			}
			if(c<x)x=c,avg=v;
		}
		if(fileSize>interval){
			rc.code(avg,RICE_MASK);
			for(int i = 0; i < inPos;) rc.code(out_pos[++i]-avg,2);
		}
		rc.flush();
	}
	for(vector<int>::iterator i = treePos.begin(); i != treePos.end(); i++){
		int tmp = *i;
		fwrite(&tmp,sizeof(int),1,outfp);
	}
	fwrite(&nowPos,sizeof(int),1,outfp);
//	if(history) delete[] history;
	nowPos=ftell(outfp);
	printf("size:%d(ratio:%.3f)\n",nowPos,(float)nowPos / fileSize * 100);
}
}
