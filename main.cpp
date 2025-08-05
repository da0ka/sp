//sp Daisuke Okanohara (VZV05226@nifty.com)
#include<iostream>
#include<stdio.h>
#include<string>
#include"sp.hpp"

typedef vector<string> vs;

const char* USAGE = "sp (e/d) [-iacps] inputfileName      please see detail -h\n";

int main(int argc, char * argv[]){
	if(argc < 3){
		if(argc == 2){
			string argv1(argv[1]);
			if(argv1 == "-h"){printf(
				"sp %s(2004-11-13)\n"
				"usage: sp e [-i] fileName\n"
				"       sp d [-acps] fileName.d\n"
				" -i    interval of compression. default is 256\n"
				"       low: partial decompression without extra decompression"
				"       high:better compression ratio"
				" -a    decompress all data\n"
				" -c    set stdout as output\n"
				" -p    partially decompress following filename.dlist\n"
				"       filename.dlist consists of start and end pair. For example,\n"
				"       100 150\n"
				"       20000 20010\n"
				" -sN-M partially decompress from Nbyte to Mbyte.\n"
				"       Default is N=0, M=fileSize\n"
				" -sN:L partially decompress from Nbyte to (N+L)byte.\n"
				"--- example ---\n"
				" sp e -i64 abcd     (compress abcd and make abcd.d interval is 64)\n"
				" sp d -a abcd.d     (decompress all. Result saves to abcd.d.tes)\n"
				" sp d -s10000-11000 (decompress from 10000 byte to 10999 byte)\n"
				" sp d -s10000:2000  (decompress from 10000 byte to 11999 byte)\n"
				" sp d -s-10000      (decompress from 0 byte to 9999 byte)\n"
				" sp d -s50000-      (decompress from 50000 byte to EOF)\n",VERSION);
				return 0;
			}
		}
		//argc error
		cout << USAGE << endl;
		return -1;
	}
	try{
		//read fileName and option
		string fileName;
		vs opt;
		bool fileNameExist = false;
		for(int i = 2; i < argc; i++){
			if(argv[i][0] == '-'){
				if(argv[i][1] == 0) throw"option error";
				opt.push_back(string((argv[i] + 1)));
			}else{
				if(fileNameExist) throw"too many fileName";
				fileName = argv[i];
				fileNameExist = true;
			}
		}
		if(!fileNameExist) throw"no fileName";
		string argv1(argv[1]);
		if(argv1 == "e"){
			//encode
			freopen("edebug.txt","wb",stderr);
			sp::encode(fileName,opt);
		}else if(argv1 == "d"){
			//decode
			freopen("ddebug.txt","wb",stderr);
			sp::decode(fileName,opt);
		}else throw"argv error";//argv error
	}
	catch(const char* errorMessage){
		printf("%s\n",errorMessage);
		return -1;
	}
	return 0;
}