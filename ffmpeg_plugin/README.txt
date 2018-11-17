# svt-hevc ffmpeg plugin installation

- git clone https://github.com/intel/SVT-HEVC
- cd SVT-HEVC
- git checkout new_api
- cd Build/linux 
- ./build release
- cd ../..
- sudo ./install_libsvt_hevc_ffmpeg.sh
- cd ..
- git clone https://github.com/FFmpeg/FFmpeg ffmpeg
- cd ffmpeg
- git reset --hard e10fe34e7c6031baa07fb3bad5c284b2911f501b
- git apply svt-hevc.patch
- ./configure --prefix=/usr --libdir=/usr/lib --enable-nonfree --enable-static --disable-shared --enable-libsvt --enable-gpl
- make -j
- sudo make install


>> ffmpeg is now built with svt-hevc, sample command line : 
./ffmpeg  -i input.mp4   -c:v libsvt_hevc -rc 1 -bitrate 10000000  -enc_p 12  -y test.265

