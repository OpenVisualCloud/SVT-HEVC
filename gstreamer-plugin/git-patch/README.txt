# svt-hevc gstreamer plugin installation

1. Build and install SVT-HEVC 
- git clone https://github.com/intel/SVT-HEVC
- cd SVT-HEVC
- mkdir build && cd build && cmake .. && make -j `nproc` && sudo make install

2. Apply SVT-HEVC plugin and enable libsvthevc to Gstreamer
- git clone https://gitlab.freedesktop.org/gstreamer/gst-build.git
- cd gst-build
- git checkout master
- meson build
- cd subprojects/gst-plugins-bad
- git apply ../SVT-HEVC/gstreamer-plugin/git-patch/0001-svthevcenc-Add-svthevc-encoder-element.patch
- cd ../../
- meson build --reconfigure
- ninja -C build

3. Verify
>> Gstreamer is now built with svt-hevc, sample command line:
- gst-launch-1.0 videotestsrc ! svthevcenc ! fakesink
- gst-launch-1.0 videotestsrc num-buffers=100 ! svthevcenc ! h265parse ! qtmux ! filesink location=test.mp4
>> If you have Gstreamer already installed, run the 'gst-uninstalled' script that lets you enter an uninstalled development environment before using the command line: 
- ninja -C build uninstalled

