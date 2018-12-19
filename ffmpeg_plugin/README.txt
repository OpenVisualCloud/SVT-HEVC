# svt-hevc ffmpeg plugin installation

1. Build and install SVT-HEVC 
- git clone https://github.com/intel/SVT-HEVC
- cd SVT-HEVC
- mkdir build && cd build && cmake .. && make -j `nproc` && sudo make install

2. Apply SVT-HEVC plugin and enable libsvthevc to FFmpeg
- git clone https://github.com/FFmpeg/FFmpeg ffmpeg
- cd ffmpeg
- git checkout release/4.1
- git apply ../SVT-HEVC/ffmpeg_plugin/0001-lavc-svt_hevc-add-libsvt-hevc-encoder-wrapper.patch
- git apply ../SVT-HEVC/ffmpeg_plugin/0002-doc-Add-libsvt_hevc-encoder-docs.patch
- export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/local/lib
- export PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:/usr/local/lib/pkgconfig
- ./configure --prefix=/usr --libdir=/usr/lib --enable-nonfree --enable-static --disable-shared --enable-libsvthevc --enable-gpl
- make -j `nproc`

3. Verify
>> ffmpeg is now built with svt-hevc, sample command line: 
./ffmpeg  -i input.mp4   -c:v libsvt_hevc -rc 1 -b:v 10M  -tune 0 -preset 9  -y test.265
./ffmpeg  -i inout.mp4 -vframes 1000 -c:v libsvt_hevc -y test.mp4

