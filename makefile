sp: main.cpp rangecoder.hpp rangecoder_util.cpp riceCoder.cpp riceCoder.hpp sp.hpp spDecode.cpp spDNode.cpp spEModel.cpp spEncode.cpp spNode.cpp
	g++ -o $@ -O9 main.cpp rangecoder_util.cpp riceCoder.cpp spDecode.cpp spDNode.cpp spEModel.cpp spEncode.cpp spNode.cpp
clean:
	rm sp