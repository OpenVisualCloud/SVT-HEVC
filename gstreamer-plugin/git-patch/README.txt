# svt-hevc gstreamer plugin installation

1. Build and install SVT-HEVC 
- git clone https://github.com/intel/SVT-HEVC
- cd SVT-HEVC
- mkdir build && cd build && cmake .. && make -j `nproc` && sudo make install

2. Apply SVT-HEVC plugin and enable libsvthevc to Gstreamer
- git clone https://gitlab.freedesktop.org/gstreamer/gst-build.git
- cd gst-build
- git checkout 1.14
- meson build
- cd subprojects/gst-plugins-bad
- git apply ../SVT-HEVC/gstreamer_plugin/0001-svth265enc-Add-svth265-encoder-element.patch
- cd ../../gst-build
- meson build --reconfigure
- ninja -C build

3. Verify
>> Gstreamer is now built with svt-hevc, sample command line:
gst-launch-1.0 videotestsrc ! svth265enc ! fakesink
gst-launch-1.0 videotestsrc num-buffers=100 ! svth265enc ! h265parse ! qtmux ! filesink location=test.mp4

