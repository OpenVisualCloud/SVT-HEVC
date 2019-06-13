# svt-hevc ffmpeg plugin installation

1. Build and install SVT-HEVC 
- git clone https://github.com/OpenVisualCloud/SVT-HEVC
- cd SVT-HEVC/Build/linux
- ./build.sh debug|release
- cd debug|release
- make install

2. Apply SVT-HEVC plugin and enable libsvthevc to FFmpeg
- git clone https://github.com/FFmpeg/FFmpeg ffmpeg
- cd ffmpeg
- git checkout release/4.1
- git apply ../SVT-HEVC/ffmpeg_plugin/*.patch

- export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/local/lib
- export PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:/usr/local/lib/pkgconfig
- ./configure --enable-libsvthevc
- make -j `nproc`

3. Verify
>> ffmpeg is now built with svt-hevc, sample command line: 
./ffmpeg -i input.mp4 -c:v libsvt_hevc -rc 1 -b:v 10M -tune 0 -preset 9 -y test.265
./ffmpeg -i input.mp4 -c:v libsvt_hevc -vframes 1000 -y test.mp4
