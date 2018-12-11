# svt-hevc ffmpeg plugin installation

1. Build and install SVT-HEVC 
- git clone https://github.com/intel/SVT-HEVC
- cd SVT-HEVC
- git checkout new_api
- mkdir build && cd build && cmake ../ && make && sudo make install

2. Apply SVT-HEVC plugin and enable libsvt to FFmpeg
- cd ..
- git clone https://github.com/FFmpeg/FFmpeg ffmpeg
- cd ffmpeg
- git apply 0001-lavc-svt_hevc-add-libsvt-hevc-encoder-wrapper.patch
- git apply 0002-doc-Add-libsvt_hevc-encoder-docs.patch
- source export_libsvt_hevc.sh
- ./configure --prefix=/usr --libdir=/usr/lib --enable-nonfree --enable-static --disable-shared --enable-libsvt --enable-gpl
- make -j `nproc`
- sudo make install

3. Verify
>> ffmpeg is now built with svt-hevc, sample command line: 
./ffmpeg  -i input.mp4   -c:v libsvt_hevc -rc 1 -c:b 10M  -enc_p 12  -y test.265
./ffmpeg  -i inout.mp4 -vframes 1000 -c:v libsvt_hevc -y test.mp4

