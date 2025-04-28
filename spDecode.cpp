/*
	sp	Flexible Data compression System. 2004
	Daisuke Okanohara (VZV05226@nifty.com)
*/
#include"sp.hpp"

namespace sp{

int totalNumber;

d_node* readTree(FILE* infp, int filePos,d_node* base, BYTE* history){
	fseek(infp,filePos,SEEK_SET);
	//int treeSize;
	//fread(&treeSize,sizeof(int),1,infp);
	riceDecode rc(RICE_MASK,infp);
	d_node* t = new d_node();
	totalNumber = 0;

	t->inputRice(rc,0);
	rangeDecoder rd;
	rd.setup(infp);
	t->inputRice4(base,rd);
	t->set_ncSkip2();
	printf("\nalloc %10dbytes",t->allocateSize());
	if(CACHE == 1){
		t->freeChildren();
	}
	return t;
}
void decode(string& fileName,vs& opt){
	FILE* infp = fopen(fileName.c_str(),"rb");
	int fileSize;
	fread(&fileSize,sizeof(int),1,infp);
	int interval;
	fread(&interval,sizeof(int),1,infp);
	if(fileName.size() <= 2 || fileName.substr(fileName.size() - 2,2) != EXT){
		cout << fileName  << endl;
		throw "fileName doesn't include .d";
	}
	//option read
	bool stdoutCheck = false;
	vector< pair<int,int> > orderList;
	for(vs::iterator i = opt.begin(); i != opt.end(); i++){
		if(*i == "p"){
			string fileName_t = fileName;
			fileName_t += "list";
			fprintf(stderr,"%s\n",fileName_t.c_str());
			ifstream tp(fileName_t.c_str());
			if(!tp){
				throw "tp open error";
			}
			int prepare = -1;
			for(;;){
				int start,last;
				if(!(tp >> start)) break;
				if(prepare >= start){
					throw "tp start is not good";
				}
				prepare = start;
				tp >> last;
				if(prepare >= last){
					printf("%d %d\n",prepare,last);
					throw "tp last is not good";
				}
				if(last >= fileSize){
					printf("%d %d\n",fileSize,last);
					throw "tp is over the fileSize";
				}
				orderList.push_back(pair<int,int>(start,last));
			}
		}
		else if(*i == "a"){
			orderList.push_back(pair<int,int>(0,fileSize));
		}
		else if(*i == "t"){
			for(int step = 0; step < fileSize;){
				orderList.push_back(pair<int,int>(step,min(step + 1024,fileSize)));
				step += 1048576;
			}
		}
		else if(*i == "c"){
			stdoutCheck = true;
		}
		else if((*i)[0] == 's'){
			//sXXXX:XXXX
			int start = -1;
			int last  = -1;
			bool colon = false;
			size_t t = i->find(':');
			if(t == -1){
				t = i->find('-');
				if(t == -1){
					throw "-s format error";
				}
			}
			else{
				colon = true;
			}
			if(!colon && t == 0){
				start = 0;
			}
			else{
				start = atoi(i->substr(1,t-1).c_str());
			}
			if(!colon && (t == i->size() - 1)){
				last = fileSize;
			}
			else{
				last  = atoi(i->substr(t+1,i->size()-t-1).c_str());
			}
			if(colon){
				last += start;
			}
			if(start < 0){
				printf("start:%d last:%d\n",start,last);
				throw "start should be greater than or equal to 0";
			}
			if(start > fileSize){
				printf("start:%d last:%d\n",start,last);
				throw "start should be smaller than fileSize";
			}
			if(last <= start){
				printf("start:%d last:%d\n",start,last);
				throw "last should be greater than start";
			}
			if(last > fileSize){
				printf("start:%d last:%d\n",start,last);
				throw "last should be smaller than or equal to fileSize";
			}
			orderList.push_back(pair<int,int>(start,last));
		}
	}
	FILE* outfp;
	if(stdoutCheck == false){
		outfp = fopen((fileName + ".tes").c_str(),"wb");
		if(outfp == 0){
			cout << "cannot open:" << fileName;
			throw "file open error";
		}
	}
	else{
		outfp = stdout;
	}
	vector<int> outPos;
	vector<int> treePos;

	if(stdoutCheck == false){
		printf("%s (original size %d) INTERVAL:%d  read parm...",(fileName.substr(0,fileName.size() - 2)).c_str(),fileSize,interval);
	}
	int rootGsize = ((fileSize + MAXBUF - 1) / MAXBUF);

	//printf("rootGsize:%d %d %d\n",rootGsize,fileSize/MAXBUF,fileSize % MAXBUF);
	{
		//read (4 + rootGsize)byte from EOF
		fseek(infp,0,SEEK_END);
		int last = ftell(infp);
		fseek(infp,(long)(last - (sizeof(int) * (rootGsize + 1))),SEEK_SET);
		for(int i = 0; i < rootGsize; i++){
			int tmp;
			fread(&tmp,sizeof(int),1,infp);
			treePos.push_back(tmp);
		}
		int firstPos;
		fread(&firstPos,sizeof(int),1,infp);
		fseek(infp,firstPos,SEEK_SET);

		riceDecode rc(RICE_MASK,infp);
		outPos.resize((fileSize / interval) + 1);
		outPos[0] = rc.unsigneddecode(RICE_MASK);
		int average = rc.unsigneddecode(RICE_MASK);

		for(int i = 1; i < (fileSize / interval) + 1; i++){
			outPos[i] = rc.decode(2) + outPos[i-1] + average;
		}
	}
	d_node* base = new d_node();
	base->nc.resize(0x100);
	for(int i = 0; i < 0x100; i++){
		base->nc[i]   = i;
	}
	base->size = 0x100;
	BYTE* history = new BYTE[MAXDEPTH];
	if(stdoutCheck == false){
		printf("done\n");
	}
	if(orderList.size() == 0){
		printf("please use -a -s -p to decompress.\n");
		return;
	}
	rangeDecoder d;
	int beforeRoot = -1;
	d_node* root   = NULL;
	for(vector< pair<int,int> > ::iterator i = orderList.begin(); i != orderList.end(); i++){
		int start = i->first;
		int last  = i->second;
		int startBlock  = start / interval;
		int startOffset = start % interval;

		if(stdoutCheck == false){
			printf("decode from:%d to:%d ",start,last);
		}
		int step = 0;
		int started = 0;
		int remain = last - start;
		int nowBlock = startBlock;
		if(start >= fileSize){
			throw "start error";
		}
		int currentRoot = start / MAXBUF;

		if(currentRoot != beforeRoot){
			root = readTree(infp,treePos[currentRoot],base,history);
			beforeRoot = currentRoot;
		}
		fseek(infp,outPos[startBlock],SEEK_SET);
		d.setup(infp);
		d_node* t = root;
		for(;;){
			fprintf(stderr,"%d\n",t->cumFreq.size() - 1);
			int c = d.getCharacterShift2(t->cumFreq,t->size,R_SHIFT);
			int cp = t->nc[c];
			step++;
			if(started == 0){
				if(step == startOffset + 1){
					started = 1;
				}
				else{
					t = t->ncSkip[c];
					continue;
				}
			}
			putc(cp,outfp);
			//printf("%d\n",last - remain + 1);
			remain--;
			//fprintf(stderr,"%d\n",remain);
			if(remain ==  0) break;
			if(step == interval){
				nowBlock++;
				step = 0;
				currentRoot = (last - remain) / MAXBUF;

				if(currentRoot != beforeRoot){

					if(CACHE == 0){
						root->freeAll();
						delete root;
					}
					root = readTree(infp,treePos[currentRoot],base,history);
					fseek(infp,outPos[nowBlock],SEEK_SET);
					beforeRoot = currentRoot;
				}
				d.setup(infp);
				t = root;
				continue;
			}
			t = t->ncSkip[c];
		}
		if(stdoutCheck == false){
			printf("done.\n");
		}
	}
	if(base) delete base;
	if(history) delete[] history;
	if(root){
		root->freeAll();
		delete root;
	}
}
}