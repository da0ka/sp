#include"sp.hpp"
namespace sp{
void encode(string& fileName,vs& opt){
	//option read
	encodeOption eo(opt);

	FILE* infp = fopen(fileName.c_str(),"rb");
	if(!infp) throw"infp open error";
	FILE* outfp;
	{
		string outfileName(fileName);
		outfileName += EXT;
		outfp = fopen(outfileName.c_str(),"wb");
		if(!outfp) throw"outfp open error";
	}
	//get filesize
	fseek(infp,0,SEEK_END);
	int fileSize = ftell(infp);
	rewind(infp);
	printf("compressing %s(%dB) ",fileName.c_str(),fileSize);
	eModel em(fileSize,min(fileSize,MAXBUF),eo);
	em.compress(infp,outfp);
}
}