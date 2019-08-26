# svt-hevc ffmpeg plugin installation

For Linux:
1. Build and install SVT-HEVC 
- git clone https://github.com/OpenVisualCloud/SVT-HEVC
- cd SVT-HEVC/Build/linux
- ./build.sh debug|release
- cd debug|release
- make install
- export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/local/lib
- export PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:/usr/local/lib/pkgconfig

For Windows:
0. Setup environment
- Install Visual Studio 2017
- Install msys2-x86_64 to C:\msys64 following http://www.msys2.org/
- Run "C:\msys64\mingw64.exe" to open a console
- Execute "pacman -S make mingw-w64-x86_64-cmake mingw-w64-x86_64-gcc mingw-w64-x86_64-yasm mingw-w64-x86_64-SDL2 perl diffutils pkgconfig git" in the console

1. Build and install SVT-HEVC
- git clone https://github.com/OpenVisualCloud/SVT-HEVC
- cd SVT-HEVC/Build/windows
- ./build.bat 2017 debug|release
- cmake --install . --config Debug|Release
- export PKG_CONFIG_PATH=/c/svt-encoders/lib/pkgconfig:$PKG_CONFIG_PATH
- pkg-config --print-errors SvtHevcEnc
- export PATH=$PATH:/c/svt-encoders/lib/

For Both:
2. Install FFmpeg with SVT-HEVC FFmpeg plugin
- git clone https://github.com/FFmpeg/FFmpeg ffmpeg
- cd ffmpeg
- git checkout release/4.2
- git am ../SVT-HEVC/ffmpeg_plugin/0001*.patch
- ./configure --enable-libsvthevc
- make -j `nproc`

3. Verify
- ./ffmpeg -i input.mp4 -c:v libsvt_hevc -rc 1 -b:v 10M -tune 0 -preset 9 -y test.265
- ./ffmpeg -i input.mp4 -c:v libsvt_hevc -vframes 1000 -y test.mp4
