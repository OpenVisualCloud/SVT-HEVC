
# Scalable Video Technology for HEVC Encoder (SVT-HEVC Encoder)

The Scalable Video Technology for HEVC Encoder (SVT-HEVC Encoder) is an HEVC-compliant encoder library core that achieves excellent density-quality tradeoffs, and is highly optimized for Intel® Xeon™ Scalable Processor and Xeon™ D processors.

This encoder has been optimized to achieve excellent performance levels using 13 density-quality presets (please refer to the user guide for more details) on a system with a dual Intel® Xeon® Scalable CPU targeting:

-  Real-time encoding of up to one 8Kp60/10-bit streams on the Platinum 8180 with M11 in the subjective quality mode

-  Real-time encoding of up to two 8Kp50/10-bit streams on the Platinum 8180 with M12 in the subjective quality mode

-  Real-time encoding of up to four 4Kp60/10-bit streams on the Gold 6148 with M12 in the subjective quality mode

-  Real-time encoding of up to six 4Kp60/10-bit streams on the Platinum 8180 with M12 in the subjective quality mode

SVT-HEVC Encoder also supports 2 modes:

-  A Subjectively optimized mode (-tune 0)

-  An Objectively optimized mode for PSNR / SSIM / VMAF benchmarking (-tune 1 (Default setting))

The encoder can also run the ABR profile below on one Intel® Xeon-D™ D-2191:


|		ABR Profile 		| 
|---------------------------------------|
|	1 x 4kp60/10-bit Stream	(@M11)	|
|	1 x 1080p60/10-bit Stream (@M10)	|
|	1 x 720p60/8-bit Stream	(@M9)	|
|	1 x 480p60/8-bit Stream	(@M9)	|
|	1 x 360p60/8-bit Stream	(@M9)	|

# License

Scalable Video Technology is licensed under BSD+Patent license. See [LICENSE](LICENSE.md) for details.

# Documentation

More details about the encoder usage can be found under:
-   [SVT-HEVC Encoder User Guide](Docs/SVT-HEVC_Encoder_User_Guide.pdf)

# System Requirements

## Operating System

SVT-HEVC may run on any Windows* or Linux* 64 bit operating systems. The list below represents the operating systems that the encoder application and library were tested and validated on:

* __Windows* Operating Systems (64-bit):__

	-  Windows* 10

	-  Windows* Server 2016

* __Linux* Operating Systems (64-bit):__

	-  Ubuntu* 16.04 Desktop LTS

	-  Ubuntu* 16.04 Server LTS

	-  Ubuntu* 18.04 Desktop LTS

	-  Ubuntu* 18.04 Server LTS

## Hardware

The SVT-HEVC Encoder library supports x86 architecture

* __CPU Requirements__

In order to achieve the performance targeted by the encoder, the specific CPU model listed above would need to be used when running the encoder. Otherwise, the encoder runs on any 5th Generation Intel Core™ Processors (formerly Broadwell) CPUs (Xeon E5-v4) or newer.

* __RAM Requirements__

In order to run the highest resolution supported by the encoder, at least 64GB of RAM is required to run a single 8kp50/10-bit encode. The encoder application will display an error if the system does not have enough RAM to support such. The following table shows the minimum amount of RAM required for some standard resolutions of 10bit video per stream:


|		Resolution 		| Minimum Footprint (GB)|
|-----------------------|-----------------------|
|		8k 			|                   64		|
|		4k 			|                   16		|
|		1080p 			|                   6           |
|		720p/1080i 		|                   4           |
|		480p 			|                   3           |

# Build and Install

## Windows* Operating Systems (64-bit):

* __Build Requirements__
	-	Visual Studio* 2017 (can be downloaded [here](https://www.visualstudio.com/vs/older-downloads/))
	-	YASM Assembler version 1.2.0 or later
	-	Download the yasm package from the following [link](http://www.tortall.net/projects/yasm/releases/vsyasm-1.2.0-win64.zip)
	-	Unzip the package and in the vsyasm.props file and make the following change:
		-	"(IntDir)"  into  "(IntDir)\%(Filename).obj"
	-	Copy the Yasm files [all 4 of them] under the bin folder of your Visual Studio* installation folder ($(VCInstallDir)) e.g.: C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\bin (the path may differ based on the version of visual studio installed)

* __Build Instructions__
	-	Open the “Build\VS\ebHevcEnc.sln” file and build the solution

* __Binaries Location__
	-	Post build, binaries can be found under Bin/Release and / or Bin/Debug

* __Installation__
-	For the binaries to operate properly on your system, the following conditions have to be met:
	-	On any of the Windows* Operating Systems listed in the OS requirements section, install Visual Studio 2017
	-	Once the installation is complete, copy the binaries to a location making sure that both the sample application “ebHevcEncApp.exe” and library “ebHevcEncLib.dll” are in the same folder.
	-	Open the command prompt window at the chosen location and run the sample application to encode.
	-	Sample application supports reading from pipe. E.g. ffmpeg -i [input.mp4] -nostdin -f rawvideo -pix_fmt yuv420p - | ebHevcEncApp.exe -i stdin -n [number_of_frames_to_encode] -w [width] -h [height].

## Linux* Operating Systems (64-bit):

* __Build Requirements__
	 -	GCC 5.4.0 or later
	 -	CMake 3.5.1 or later
	 -	YASM Assembler version 1.2.0 or later

* __Build Instructions__
	 -	cd Build/linux
	 -	./build.sh <release | debug> (if none specified, both release and debug will be built)

* __Sample Binaries location__
	 -	Binaries can be found under Bin/Release and / or Bin/Debug

* __Installation__
For the binaries to operate properly on your system, the following conditions have to be met:
	-	On any of the Linux* Operating Systems listed above, copy the binaries under a location of your choice.
	-	Change the permissions on the sample application “HevcEncoderApp” executable by running the command: 				chmod +x HevcEncoderApp
	-	cd into your chosen location
	-	Run the sample application to encode.
	-	Sample application supports reading from pipe. E.g. ffmpeg -i [input.mp4] -nostdin -f rawvideo -pix_fmt yuv420p - | ./HevcEncoderApp -i stdin -n [number_of_frames_to_encode] -w [width] -h [height].

# Demo features and limitations

-  **Resolution support:** This version supports only multiple-of-8 resolutions in width for 8-bit video input and in width and height for 10-bit video input.

-  **VBR BRC mode:** The VBR functionality implemented in SVT-HEVC Encoder is a demo feature to allow for an easier integration of product level BRC. The algorithm implemented would allow the encoder to generate an output bit stream matching, with a best effort, the target bitrate. The algorithm does not guarantee a certain maximum bitrate or maximum buffer size [does not follow HRD compliance]. When set to encode in VBR mode, the encoder does not produce a bit-exact output from one run to another.

-  **Speed Control output:** The speed control functionality implemented for SVT-HEVC Encoder is a demo feature showcasing the capability of the library to adapt to the resources available on the fly in order to generate the best possible video quality while maintaining a real-time encoding speed. When set to use the Speed Control mode, the encoder does not produce a bit-exact output from one run to another.

-  **Multi-instance support:** The multi-instance functionality is a demo feature implemented in the SVT-HEVC Encoder sample application as an example of one sample application using multiple encoding libraries. Encoding using the multi-instance support is limited to only 6 simultaneous streams.

-  **Separate Fields:** Using the separate fields functionality migh result in a corrupted video output.

# How to Contribute

We welcome community contributions to the Scalable Video Technology. Thank you for your time! By contributing to the project, you agree to the license and copyright terms therein and to the release of your contribution under these terms.

## Contribution process

-  Validate that your changes do not break a build

-  Perform smoke tests and ensure they pass

-  Submit a pull request for review to the maintainer

# How to Report Bugs and Provide Feedback

Use the "Issues" tab on Github
