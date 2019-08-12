# Scalable Video Technology for HEVC Encoder (SVT-HEVC Encoder)

[![AppVeyor Build Status](https://ci.appveyor.com/api/projects/status/github/openvisualcloud/SVT-HEVC?branch=master&svg=true)](https://ci.appveyor.com/project/openvisualcloud/SVT-HEVC)
[![Travis Build Status](https://travis-ci.com/OpenVisualCloud/SVT-HEVC.svg?branch=master)](https://travis-ci.com/OpenVisualCloud/SVT-HEVC)
[![Coverage Status](https://coveralls.io/repos/github/openvisualcloud/SVT-HEVC/badge.svg?branch=master)](https://coveralls.io/github/openvisualcloud/SVT-HEVC?branch=master)

The Scalable Video Technology for HEVC Encoder (SVT-HEVC Encoder) is an HEVC-compliant encoder library core that achieves excellent density-quality tradeoffs, and is highly optimized for Intel® Xeon™ Scalable Processor and Xeon™ D processors.

The whitepaper for SVT-HEVC can be found here: <https://01.org/svt>

This encoder has been optimized to achieve excellent performance levels using 12 density-quality presets (please refer to the user guide for more details).

## License

Scalable Video Technology is licensed under the OSI-approved BSD+Patent license. See [LICENSE](LICENSE.md) for details.

## Documentation

More details about the encoder usage can be found under:

- [svt-hevc-encoder-user-guide](Docs/svt-hevc_encoder_user_guide.md)

## System Requirements

### Operating System

SVT-HEVC may run on any Windows* or Linux* 64 bit operating systems. The list below represents the operating systems that the encoder application and library were tested and validated on:

- __Windows* Operating Systems (64-bit):__

  - Windows Server 2016

- __Linux* Operating Systems (64-bit):__

  - Ubuntu 16.04 Server LTS

  - Ubuntu 18.04 Server LTS

  - CentOS 7.4/7.5/7.6

### Hardware

The SVT-HEVC Encoder library supports x86 architecture

- __CPU Requirements__

In order to achieve the performance targeted by the encoder, the specific CPU model listed above would need to be used when running the encoder. Otherwise, the encoder runs on any 5th Generation Intel Core™ Processors (formerly Broadwell) CPUs (Xeon E5-v4) or newer.

- __RAM Requirements__

In order to run the highest resolution supported by the encoder, at least 64GB of RAM is required to run a single 8kp50/10-bit encode. The encoder application will display an error if the system does not have enough RAM to support such. The following table shows the minimum amount of RAM required for some standard resolutions of 10bit video per stream:

| Resolution | Minimum Footprint (GB) |
| ---------- | ---------------------- |
| 8k         | 64                     |
| 4k         | 16                     |
| 1080p      | 6                      |
| 720p/1080i | 4                      |
| 480p       | 3                      |

## Build and Install

### Windows* Operating Systems (64-bit)

- __Build Requirements__
  - Visual Studio* 2017 (download [here](https://www.visualstudio.com/vs/older-downloads/)) or 2019 (download [here](https://visualstudio.microsoft.com/downloads/))
  - CMake 3.14 or later (download [here](https://github.com/Kitware/CMake/releases/download/v3.14.4/cmake-3.14.4-win64-x64.msi))
  - YASM Assembler version 1.2.0 or later
    - Download the yasm exe from the following [link](http://www.tortall.net/projects/yasm/releases/yasm-1.3.0-win64.exe)
    - Rename yasm-1.3.0-win64.exe to yasm.exe
    - Copy yasm.exe into a location that is in the PATH environment variable

- __Build Instructions__
  - Build the project by following the steps below in a windows command prompt:
    - In the main repository directory, cd to `<repo dir>\Build\windows`
    - Run `build.bat [2019|2017|2015]` [This will automatically generate and build the project]
  - To Build the project using a generator other than Visual Studio
    - run `build.bat [ninja|msys|mingw|unix]` instead of the second command
    - Note: These are not officially supported and thus are not displayed in the help message.

- __Binaries Location__
  - Binaries can be found under `<repo dir>\Bin\Release` or `<repo dir>\Bin\Debug`, depending on whether Debug or Release was selected

- __Installation__\
    For the binaries to operate properly, the following conditions have to be met:
  - On any of the Windows* Operating Systems listed in the OS requirements section, install Visual Studio* 2017 or 2019
  - Once the build is complete, copy the binaries to a location making sure that both the application `SvtHevcEncApp.exe` and library `SvtHevcEnc.dll` are in the same folder.
  - Open the command prompt at the chosen location and run the application to encode. `SvtHevcEncApp.exe -i [in.yuv] -w [width] -h [height] -b [out.265]`
  - The application also supports reading from pipe. E.g. `ffmpeg -i [input.mp4] -nostdin -f rawvideo -pix_fmt yuv420p - | SvtHevcEncApp.exe -i stdin -n [number_of_frames_to_encode] -w [width] -h [height]`

### Linux* Operating Systems (64-bit)

- __Build Requirements__
  - GCC 5.4.0 or later
  - CMake 3.5.1 or later
  - YASM Assembler version 1.2.0 or later

- __Build Instructions__
  - In the main repository, run either the provided build script

    ``` bash
    cd Build/linux
    ./build.sh [release|debug] [static|shared] [install]
    # Requires sudo permission for installing
    # Run './build.sh -h' to see the full help
    ```

  - or run the commands directly

    ``` bash
    mkdir build && cd build && cmake .. -DCMAKE_BUILD_TYPE=<Release|Debug> && make -j $(nproc) && sudo make install
    ```

- __Sample Binaries location__
  - Binaries can be found under `$REPO_DIR/Bin/Release`

- __Installation__\
For the binaries to operate properly, the following conditions have to be met:
  - On any of the Linux* Operating Systems listed above, copy the binaries under a location of your choice.
  - Change the permissions on the sample application “SvtHevcEncApp” executable by running the command: `chmod +x SvtHevcEncApp`
  - cd into your chosen location
  - Run the sample application to encode. `./SvtHevcEncApp -i [in.yuv] -w [width] -h [height] -b [out.265]`
  - Sample application supports reading from pipe. E.g. `ffmpeg -i [input.mp4] -nostdin -f rawvideo -pix_fmt yuv420p - | ./SvtHevcEncApp -i stdin -n [number_of_frames_to_encode] -w [width] -h [height]`

## Demo features and limitations

- **VBR BRC mode:** \
  The VBR functionality implemented in SVT-HEVC Encoder is a demo feature to allow for an easier integration of product level BRC. The algorithm implemented would allow the encoder to generate an output bit stream matching, with a best effort, the target bitrate. The algorithm does not guarantee a certain maximum bitrate or maximum buffer size [does not follow HRD compliance]. When set to encode in VBR mode, the encoder does not produce a bit-exact output from one run to another.

- **Speed Control output:** \
  The speed control functionality implemented for SVT-HEVC Encoder is a demo feature showcasing the capability of the library to adapt to the resources available on the fly in order to generate the best possible video quality while maintaining a real-time encoding speed. When set to use the Speed Control mode, the encoder does not produce a bit-exact output from one run to another.

- **Multi-instance support:** \
  The multi-instance functionality is a demo feature implemented in the SVT-HEVC Encoder sample application as an example of one sample application using multiple encoding libraries. Encoding using the multi-instance support is limited to only 6 simultaneous streams. For example two channels encoding on Windows: SvtHevcEncApp.exe -nch 2 -c firstchannel.cfg secondchannel.cfg

## How to Contribute

We welcome community contributions to the SVT-HEVC Encoder. Thank you for your time! By contributing to the project, you agree to the license and copyright terms therein and to the release of your contribution under these terms.

### Contribution process

- Follow the [coding_guidelines](STYLE.md)
- Validate that your changes do not break a build
- Perform smoke tests and ensure they pass
- Submit a pull request for review to the maintainer

### How to Report Bugs and Provide Feedback

Use the [Issues](https://github.com/OpenVisualCloud/SVT-HEVC/issues) tab on Github

## IRC

`#svt` on Freenode. Join via [Freenode Webchat](https://webchat.freenode.net/?channels=svt) or use your favorite IRC client.

## Notices and Disclaimers

The notices and disclaimers can be found [here](NOTICES.md)
