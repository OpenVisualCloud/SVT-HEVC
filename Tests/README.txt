HOW TO USE:
1. Build SVT encoder and place the executable in the folder specified under "ENC_PATH"
2. Download the reference decoder from https://hevc.hhi.fraunhofer.de/
3. Build the decoder and place it in the folder specified under "TOOLS_PATH"
4. Run script

Test input options:
- python SVT-HEVC_FunctionalTests.py Fast
- python SVT-HEVC_FunctionalTests.py Nightly
- python SVT-HEVC_FunctionalTests.py Full
* Fast test takes around 2 hours
* Nightly test takes around 18 hours
* Full test takes around 2 days
* Estimated time is based on tests ran on Intel Xeon Gold 6140 CPU w/ 94.7 GB memory
