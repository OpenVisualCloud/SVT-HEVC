cp ../Source/API/* /usr/include/
cp ../Bin/Release/libSvtHevcEnc.so /usr/lib/
cp svt.pc /usr/lib/pkgconfig/ 
export LD_LIBRARY_PATH=/usr/lib
export PKG_CONFIG_PATH=/usr/lib/pkgconfig