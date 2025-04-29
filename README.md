# SP partial decodable compression
SP is a data compression program that uses Static PPM.
Compared to conventional data compression methods, it has the following features:
* You can restore any part of it without restoring the whole thing
* Compression ratio is similar to bzip2 for large files, and similar to gzip for small files
* The amount of calculation and space required for the restoration process is smaller than that of conventional high compression methods(BWT and PPM)

SP was developed for research purposes, and is not intended to be used as a general compression software. Therefore, there is a possibility that compressed files cannot be restored, or in the worst case, files may be erased. Please do not use it for anything other than research purposes. The author is not responsible for any damages caused by the use of this program(SP). Please be aware of this.
## How to use
```
usage:  sp e [-i] fileName
        sp d [-acps] fileName.d
 -i    interval of compression. default is 256
       low: partial decompression without extra decompression
       high: better compression ratio
 -a    decompress all data
 -c    set stdout as output
 -p    partially decompress following filename.dlist
       filename.dlist consists of start and end pair. For example,
       100 150
       20000 20010
 -sN-M partially decompress from N byte to M byte.
       Default is N=0, M=fileSize
 -sN:L partially decompress from Nbyte to (N+L)byte.
--- example ---
 sp e -i64 abcd     (compress abcd and make abcd.d interval is 64)
 sp d -a abcd.d     (decompress all. Result saves to abcd.d.tes)
 sp d -s10000-11000 (decompress from 10000 byte to 10999 byte)
 sp d -s10000:2000  (decompress from 10000 byte to 11999 byte)
 sp d -s-10000      (decompress from 0 byte to 9999 byte)
 sp d -s50000-      (decompress from 50000 byte to EOF)
```
