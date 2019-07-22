## Copyright(c) 2018 Intel Corporation
## SPDX - License - Identifier: BSD - 2 - Clause - Patent

from __future__ import print_function
import platform
import ctypes
import os
import subprocess
import itertools
import random
import math
import filecmp
import time
import glob
import sys

LINUX_PLATFORM_STR    = "Linux"
WINDOWS_PLATFORM_STR  = "Windows"
platform = platform.system()

if platform == WINDOWS_PLATFORM_STR:
    SEM_NOGPFAULTERRORBOX = 0x0002
    ctypes.windll.kernel32.SetErrorMode(SEM_NOGPFAULTERRORBOX);
    subprocess_flags = 0x8000000
    slash = '\\'
    exe_name = 'SvtHevcEncApp.exe'
    dec_exe = 'TAppDecoder.exe'
else:
    slash = '/'
    exe_name = 'SvtHevcEncApp'
    dec_exe = 'TAppDecoder'

def get_test_mode(mode):
    if len(mode) <= 1:
        print("Running default mode: Fast")
        return 0
    test_mode = mode[1]
    if test_mode == "fast" or test_mode == "Fast":
        print("Running mode: Fast")
        return 0
    elif test_mode == "nightly" or test_mode == "Nightly":
        print("Running mode: Nightly")
        return 1
    elif test_mode == "full" or test_mode == "Full":
        print("Running mode: Full")
        return 2
    else:
        print("Mode not recognized")
        print("Running default mode: Fast")
        return 0

DEBUG_MODE = 0 # For debugging purposes

##--------------------- TEST SETTINGS --------------------##
ENC_PATH = "encoders"
BIN_PATH = "bitstreams"
YUV_PATH = "yuvs"
TOOLS_PATH = "tools"

TEST_CONFIGURATION = 0 # 0 - Validation Test, 1 - Speed Test (Refer to Validation/Speed Test specific configurations)

#------------- Validation Test Specific -------------#
VALIDATION_TEST_MODE = get_test_mode(sys.argv) # 0 - Fast Test, 1 - Overnight Test, 2- Full Test
QP_VBR_MODE = 0 # 0 - Both QP and VBR, 1 - QP Only, 2 - VBR Only

VALIDATION_TEST_SEQUENCES = [
'Netflix_Crosswalk_3840x2160_10bit_60Hz_P420',
'Fallout4_1920x1080_8bit_60Hz_P420',
'DucksTakeOff_1280x720_8bit_50Hz_P420',
'ParkJoy_864x480_10bit_50Hz_P420'
]

#-------------  Speed Test Specific -------------#
# Number of channels is enc_mode and resolution specific
SPEED_ENC_MODES = [0,6,9]

# Speed Test Sequences, use 'qp' and 'tbr' to specify to run in Fixed QP or VBR Mode
SPEED_TEST_SEQUENCES = [
{'name': 'Netflix_Boat_4096x2160_10bit_60Hz_P420', 'tbr': 13000},
{'name': 'Netflix_FoodMarket2_4096x2160_10bit_60Hz_P420_2bitspack', 'tbr': 13000},
{'name': 'Netflix_Narrator_4096x2160_10bit_60Hz_P420', 'tbr': 13000},
{'name': 'Netflix_RitualDance_4096x2160_10bit_60Hz_P420', 'tbr': 13000},
{'name': 'Netflix_Tango_4096x2160_10bit_60Hz_P420_2bitspack', 'tbr': 13000},
{'name': 'EuroTruckSimulator2_003_1920x1080_8bit_60Hz_P420', 'tbr': 7500},
{'name': 'Fallout4_003_1920x1080_8bit_60Hz_P420', 'tbr': 7500},
{'name': 'GTAV_005_1920x1080_8bit_60Hz_P420', 'tbr': 7500},
{'name': 'RUST_004_1920x1080_8bit_60Hz_P420', 'tbr': 7500},
{'name': 'WITCHER3_007_1920x1080_8bit_60Hz_P420', 'tbr': 7500},
{'name': 'EuroTruckSimulator2_003_1280x720_8bit_60Hz_P420', 'tbr': 4500},
{'name': 'Fallout4_003_1280x720_8bit_60Hz_P420', 'tbr': 4500},
{'name': 'GTAV_005_1280x720_8bit_60Hz_P420', 'tbr': 4500},
{'name': 'RUST_004_1280x720_8bit_60Hz_P420', 'tbr': 4500},
{'name': 'WITCHER3_007_1280x720_8bit_60Hz_P420', 'tbr': 4500},
{'name': 'ParkJoy_864x480_10bit_50Hz_P420', 'tbr': 2500},
{'name': 'DucksTakeOff_864x480_8bit_50Hz_P420', 'tbr': 2500},
{'name': 'CrowdRun_864x480_8bit_50Hz_P420', 'tbr': 2500},
{'name': 'InToTree_864x480_8bit_50Hz_P420', 'tbr': 2500},
{'name': 'OldTownCross_864x480_8bit_50Hz_P420', 'tbr': 2500},
]
##--------------------------------------------------------##

##-------------- TEST MODE SPECIFIC SETTINGS -------------##
if VALIDATION_TEST_MODE == 0:
    ENC_MODES                       = [1,4,8,11]
    NUM_FRAMES                      = 20
    QP_ITERATIONS                   = 1
    VBR_ITERATIONS                  = 1
    SA_ITER                         = 1 # Search Area WidthxHeight per test
    LAD_ITER                        = 1 # LAD per test
    INTRA_PERIOD_ITER               = 1 # Intra Period per test
    WH_ITER                         = 1 # WidthxHeight per test
elif VALIDATION_TEST_MODE == 1:
    ENC_MODES                       = [0,1,2,3,4,5,6,7,8,9,10,11]
    NUM_FRAMES                      = 40
    QP_ITERATIONS                   = 1
    VBR_ITERATIONS                  = 1
    SA_ITER                         = 1 # Search Area WidthxHeight per test
    LAD_ITER                        = 1 # LAD per test
    INTRA_PERIOD_ITER               = 1 # Intra Period per test
    WH_ITER                         = 1 # WidthxHeight per test
elif VALIDATION_TEST_MODE == 2:
    ENC_MODES                       = [0,1,2,3,4,5,6,7,8,9,10,11]
    NUM_FRAMES                      = 40
    QP_ITERATIONS                   = 2
    VBR_ITERATIONS                  = 2
    SA_ITER                         = 2 # Search Area WidthxHeight per test
    LAD_ITER                        = 2 # LAD per test
    INTRA_PERIOD_ITER               = 2 # Intra Period per test
    WH_ITER                         = 2 # WidthxHeight per test
##--------------------------------------------------------##

##-------------------- Global Defines --------------------##
MIN_WIDTH                       = 832
MAX_WIDTH                       = 4096
MIN_HEIGHT                      = 480
MAX_HEIGHT                      = 2304
MIN_QP                          = 30
MAX_QP                          = 51
MIN_BR                          = 1000
MAX_BR                          = 10000000
##--------------------------------------------------------##

if QP_VBR_MODE == 0:
    QP_VBR_COMBINATION = [0, 1]
elif QP_VBR_MODE == 1:
    QP_VBR_COMBINATION = [0]
elif QP_VBR_MODE == 2:
    QP_VBR_COMBINATION = [1]

class EB_Test(object):
    # Initialization parameters for folders
    def __init__(self,
                 encoder_path,
                 bitstream_path,
                 yuv_path,
                 tools_path):

        self.yuv_path       = yuv_path
        self.encoder_path   = encoder_path
        self.bitstream_path = bitstream_path
        self.tools_path     = tools_path

        if not os.path.exists(bitstream_path):
            os.mkdir(bitstream_path)

        if not os.path.exists('qp_files'):
            os.mkdir('qp_files')
        else:
            files = glob.glob('qp_files' + slash + '*.qpfile')
            for f in files:
                if os.path.exists(f):
                    os.remove(f)

    def get_default_params(self):
        # Default encoding parameters
        encoder_bit_depth                   = 8
        compressed_ten_bit_format           = 0
        width                               = 832
        height                              = 480
        frame_rate                          = 60
        intra_period                        = 31
        frame_to_be_encoded                 = NUM_FRAMES

        # Dictionary of all input parameters
        default_enc = { 'encoder_dir'                       : self.encoder_path,
                        'bitstream_dir'                     : self.bitstream_path,
                        'yuv_dir'                           : self.yuv_path,
                        'tools_dir'                         : self.tools_path,
                        'encoder_bit_depth'                 : encoder_bit_depth,
                        'width'                             : width,
                        'height'                            : height,
                        'frame_rate'                        : frame_rate,
                        'intra_period'                      : intra_period,
                        'frame_to_be_encoded'               : frame_to_be_encoded,
                        }
        return default_enc

    def get_stream_info(self, seq_name):
        enc_params = {}
        if "_4096x2160_" in seq_name:
            width = 4096
            height = 2160
            enc_params.update({'width' : width, 'height': height})
        elif "_3840x2160_" in seq_name:
            width = 3840
            height = 2160
            enc_params.update({'width' : width, 'height': height})
        elif "_1920x1080_" in seq_name:
            width = 1920
            height = 1080
            enc_params.update({'width' : width, 'height': height})
        elif "_1920x540_" in seq_name:
            width = 1920
            height = 540
            enc_params.update({'width' : width, 'height': height})
        elif "_1280x720_" in seq_name:
            width = 1280
            height = 720
            enc_params.update({'width' : width, 'height': height})
        elif "_864x480_" in seq_name:
            width = 864
            height = 480
            enc_params.update({'width' : width, 'height': height})
        elif "_832x480_" in seq_name:
            width = 832
            height = 480
            enc_params.update({'width' : width, 'height': height})

        if "_8bit_" in seq_name:
            bit_depth = 8
            enc_params.update({'encoder_bit_depth' : bit_depth})
        elif "_10bit_" in seq_name:
            bit_depth = 10
            enc_params.update({'encoder_bit_depth' : bit_depth})

        if "_2bitspacked" in seq_name:
            compressed_ten_bit = 1
            enc_params.update({'compressed_ten_bit_format' : compressed_ten_bit})
        else:
            compressed_ten_bit = 0
            enc_params.update({'compressed_ten_bit_format' : compressed_ten_bit})

        if "Hz_" in seq_name:
            frame_rate = seq_name[seq_name[:seq_name.rfind("Hz_")].rfind("_")+1:seq_name.rfind("Hz_")]
            if frame_rate == 30:
                intra_period = 31
            else:
                intra_period = int(int(frame_rate)/8)*8 - 1
            enc_params.update({'frame_rate' : frame_rate, 'intra_period': intra_period})
        return enc_params

    def get_param_tokens(self):
        # Dictionary of all input parameters
        default_tokens = {
                        'buffered_input'                    : '-nb',
                        'compressed_ten_bit_format'         : '-compressed-ten-bit-format',
                        'interlaced_video'                  : '-interlaced-video',
                        'deinterlace_input'                 : '-separate-fields',
                        'video_usability_info'              : '-vid-info',
                        'high_dyanmic_range_input'          : '-hdr',
                        'profile'                           : '-profile',
                        'tier'                              : '-tier',
                        'level'                             : '-level',
                        'enc_mode'                          : '-encMode',
                        'qp'                                : '-q',
                        'rc'                                : '-rc',
                        'tbr'                               : '-tbr',
                        'IntraRefreshType'                  : '-irefresh-type',
                        'use_qp_file'                       : '-use-q-file',
                        'qp_file_name'                      : '-qp-file',
                        'HierarchicalLevels'                : '-hierarchical-levels',
                        'PredStructure'                     : '-pred-struct',
                        'BaseLayerSwitchMode'               : '-base-layer-switch-mode',
                        'LoopFilterDisable'                 : '-dlf',
                        'SAO'                               : '-sao',
                        'ImproveSharpness'                  : '-sharp',
                        'brr'                               : '-brr',
                        'ConstrainedIntra'                  : '-constrd-intra',
                        'SceneChangeDetection'              : '-scd',
                        'LookAheadDistance'                 : '-lad',
                        'DefaultMeHme'                      : '-use-default-me-hme',
                        'HME'                               : '-hme',
                        'SearchAreaWidth'                   : '-search-w',
                        'SearchAreaHeight'                  : '-search-h',
                        }
        return default_tokens

    # Assemble the command line
    def get_enc_cmd(self, enc_param, yuv_name, bitstream_name, num_channels = 1):
        all_tokens = self.get_param_tokens()
        # command line necessary for the encoder to work
        enc_cmd =   ( enc_param['encoder_dir'] + slash + exe_name)
        if num_channels != 1:
            enc_cmd += (' -nch ') + str(num_channels)
        enc_cmd +=  (' -i ')
        for count in range (0, num_channels):
            enc_cmd += (enc_param['yuv_dir'] + slash + yuv_name + '.yuv ')
        enc_cmd +=  (' -b ')
        for count in range (0, num_channels):
            if count == 0:
                enc_cmd += (enc_param['bitstream_dir'] + slash + bitstream_name + '.265 ')
            else:
                enc_cmd += (enc_param['bitstream_dir'] + slash + bitstream_name + '_' + str(count) + '.265 ')
        enc_cmd +=  (' -errlog ')
        for count in range (0, num_channels):
            if count == 0:
                enc_cmd += (enc_param['bitstream_dir'] + slash + bitstream_name + '.errlog ')
            else:
                enc_cmd += (enc_param['bitstream_dir'] + slash + bitstream_name + '_' + str(count) + '.errlog ')
        enc_cmd +=  (' -w ')
        for count in range (0, num_channels):
            enc_cmd += (str(enc_param['width']) + ' ')
        enc_cmd +=  (' -h ')
        for count in range (0, num_channels):
            enc_cmd += (str(enc_param['height']) + ' ')
        enc_cmd +=  (' -bit-depth ')
        for count in range (0, num_channels):
            enc_cmd += (str(enc_param['encoder_bit_depth']) + ' ')
        enc_cmd +=  (' -fps ')
        for count in range (0, num_channels):
            enc_cmd += (str(enc_param['frame_rate']) + ' ')
        enc_cmd +=  (' -intra-period ')
        for count in range (0, num_channels):
            enc_cmd += (str(enc_param['intra_period']) + ' ')
        enc_cmd +=  (' -n ')
        for count in range (0, num_channels):
            enc_cmd += (str(enc_param['frame_to_be_encoded']) + ' ')

        # optional command line
        for tokens in all_tokens:
            if tokens in enc_param:
                enc_cmd += (' ' + all_tokens[tokens] + ' ')
                if isinstance(enc_param[tokens], list):
                    for count in range (0, num_channels):
                        for items in enc_param[tokens]:
                            enc_cmd += (str(items) + ' ')
                else:
                    for count in range (0, num_channels):
                        enc_cmd += (str(enc_param[tokens]) + ' ')

        # command line to produce output
        enc_cmd += (' > ' + enc_param['bitstream_dir'] + slash + bitstream_name + '.txt')
        return enc_cmd

    def get_test_params(self, seq, combination_test_params):
        test_param = []
        param_value = []
        param_name = []
        levels = len(combination_test_params)
        for params in combination_test_params:
            param_value.append(combination_test_params[params])
            param_name.append(params)
        new_list = list(itertools.product(*param_value))
        for item in new_list:
            param_dict = {}
            list_index = 0
            for name in param_name:
                param_dict.update({name: item[list_index]})
                list_index = list_index + 1
            test_param.append((seq, param_dict))
        return test_param

    def split_search_region(self, total_search_area, iteration):
        search_area = []
        total_searched = 0
        for width_index in range(0,iteration,1):
            search_area.append(int(math.floor(total_search_area/float(iteration))))
            total_searched = total_searched + int(math.floor(total_search_area/float(iteration)))
        remainder = total_search_area - total_searched
        while (remainder != 0):
            for index in range(iteration - 1, 0, -1):
                search_area[index] = search_area[index] + 1
                remainder = remainder - 1
                if remainder <= 0:
                    break
        return search_area


    # Check if sequence and combination is supported
    def check_seq_support(self, test_name, seq, enc_params):
        width       = enc_params['width']
        height      = enc_params['height']
        bitdepth      = enc_params['encoder_bit_depth']
        # Encoder constraints - if not followed, encoder will not work
        if 'enc_mode' in enc_params:
            encMode     = enc_params['enc_mode']
            # Check if encMode is valid
            if encMode >= 12:
                return -1
            if width*height < 3840*2160:
                if encMode == 10:
                    if width*height < 1920*1080:
                        return -1
        if width %2 != 0 or height %2 != 0:
            return -1
        if bitdepth == 10 and height %8 != 0:
            return -1
        # Test specific constraints
        if test_name == 'unpacked_test':
            if bitdepth == 8:
                return -1
        if test_name == 'defield_test':
            if bitdepth == 10:
                return -1
            if encMode == 10:
                if width*height == 1920*1080:
                    return -1
        if test_name == 'enc_struct_test':
            hierar_levels   = enc_params['HierarchicalLevels']
            pred_struct   = enc_params['PredStructure']
            base_sw_mode   = enc_params['BaseLayerSwitchMode']
            if base_sw_mode == 1 and pred_struct != 2:
                return -1
            if width*height >= 1920*1080:
                return -1
        if test_name == 'qp_file_test':
            if 'rc' in enc_params:
                rc = enc_params['rc']
                if rc == 1:
                    return -1
        # VALIDATION_TEST_MODE dependent settings
        if VALIDATION_TEST_MODE == 0:
            if 'enc_mode' in enc_params:
                encMode     = enc_params['enc_mode']
                if encMode >= 0 and encMode <= 3:
                    if width*height >= 1920*1080:
                        return -1
                elif encMode >= 4 and encMode <= 7:
                    if width*height > 1920*1080:
                        return -1
        return 0

    def get_width_height(self):
        widths = []
        heights = []
        width_bin_size = (float(MAX_WIDTH) - float(MIN_WIDTH))/float(WH_ITER)
        height_bin_size = (float(MAX_HEIGHT) - float(MIN_HEIGHT))/float(WH_ITER)
        for count in range(WH_ITER):
            width = int((random.randint(MIN_WIDTH + int((count)*width_bin_size), MIN_WIDTH + int((count+1)*width_bin_size))/2))*2
            widths.append(width)
            height = int((random.randint(MIN_HEIGHT + int((count)*height_bin_size), MIN_HEIGHT + int((count+1)*height_bin_size))/8))*8
            heights.append(height)
        return widths, heights

    def generate_qp_file(self, bitstream_name, num_frames):
        found = 0
        qp_file_name = bitstream_name + '.qpfile'
        for i in range(num_frames):
            print(random.randint(MIN_QP,MAX_QP), file=open('qp_files' + slash + qp_file_name, 'a'))
        return qp_file_name

    # Test to Compare exactness between bitstreams
    def run_test(self, test_name, test_params, enc_params, VBR, COMPARE):
        total_tests = 0
        passed_tests = 0
        if VBR == 0:
            iter_list = [random.randint(MIN_QP,MAX_QP) for x in range(QP_ITERATIONS)]
        else:
            iter_list = [random.randint(MIN_BR,MAX_BR) for x in range(VBR_ITERATIONS)]

        for enc_mode in ENC_MODES:
            for iter in iter_list: # QP or Bitrate
                compare_bitstream = ""
                bitstream_name = ""
                for params in test_params:
                    seq_name = params[0]
                    test_cond = params[1]
                    enc_params.update(self.get_stream_info(seq_name))
                    enc_params.update({'enc_mode': enc_mode})
                    bitstream_name = test_name + '_M' + str(enc_mode) + '_' + seq_name
                    if VBR == 0:
                        enc_params.update({'rc': 0, 'qp': iter})
                        bitstream_name = bitstream_name + '_Q' + str(iter)
                    else:
                        enc_params.update({'rc': 1, 'tbr': iter})
                        bitstream_name = bitstream_name + '_TBR' + str(iter)
                    for cond in test_cond:
                        enc_params.update({cond: test_cond[cond]})
                        if not isinstance(test_cond[cond], list):
                            bitstream_name = bitstream_name + '_' + str(test_cond[cond])
                    # Check if sequence is supported with given combinations
                    error = self.check_seq_support(test_name, seq_name, enc_params)
                    if error != 0:
                        continue
                    # Test Specific
                    if test_name == 'defield_test':
                        if '_fields' in seq_name:
                            height = enc_params['height']/2
                            nframes = enc_params['frame_to_be_encoded']*2
                            enc_params.update({'height': height, 'frame_to_be_encoded': nframes})
                        else:
                            nframes = NUM_FRAMES
                            enc_params.update({'frame_to_be_encoded': nframes})
                    elif test_name == 'qp_file_test':
                        qp_file_name = self.generate_qp_file(bitstream_name, enc_params['frame_to_be_encoded'])
                        enc_params.update({'qp_file_name': 'qp_files' + slash + qp_file_name})
                    enc_cmd = self.get_enc_cmd(enc_params, seq_name, bitstream_name)
                    print(enc_cmd, file=open(test_name + '.txt', 'a'))
                    if DEBUG_MODE == 0:
                        exit_code = subprocess.call(enc_cmd, shell = True)
                        if exit_code == 0 and test_name == 'decode_test':
                            dec_cmd = enc_params['tools_dir'] + slash + dec_exe + " -b " + enc_params['bitstream_dir'] + slash + bitstream_name + '.265 > NUL'
                            print(dec_cmd, file=open(test_name + '.txt', 'a'))
                            exit_code = subprocess.call(dec_cmd, shell = True)
                    else:
                        continue
                    if COMPARE == 0:
                        total_tests = total_tests + 1
                        if exit_code == 0:
                            print('------------Passed-------------', file=open(test_name + '.txt', 'a'))
                            passed_tests = passed_tests + 1
                        else:
                            print('------------Failed-------------', file=open(test_name + '.txt', 'a'))
                            continue
                    else:
                        if exit_code != 0:
                            print('----------Enc Error------------', file=open(test_name + '.txt', 'a'))
                            continue
                        if compare_bitstream != "":
                            exit_code = filecmp.cmp(enc_params['bitstream_dir'] + slash + compare_bitstream + '.265', enc_params['bitstream_dir'] + slash + bitstream_name + '.265')
                            if exit_code == True:
                                print('------------Passed-------------', file=open(test_name + '.txt', 'a'))
                                passed_tests = passed_tests + 1
                            else:
                                print('------------Failed-------------', file=open(test_name + '.txt', 'a'))
                                break
                        else:
                            total_tests = total_tests + 1
                    # TODO: COPY BITSTREAM TO NEW LOCATION
                    if bitstream_name != "":
                        compare_bitstream = bitstream_name
        return total_tests, passed_tests

    def run_functional_tests(self, seq_list, test_name, combination_test_params):
        # Get default encoding params
        enc_params = self.get_default_params().copy()
        # Print Test Information:
        total_test = 0
        total_passed = 0
        if os.path.exists(test_name + '.txt'):
            return 0, 0
        print ("Running Test: " + test_name)
        print ("---------------------------------------", file=open(test_name + '.txt', 'w'))
        print ("Test Name: " + test_name, file=open(test_name + '.txt', 'a'))
        for seq in seq_list:
            # Run test
            test_params = self.get_test_params(seq, combination_test_params)
            for VBR in QP_VBR_COMBINATION:
                num_tests, num_passed = self.run_test(test_name, test_params, enc_params, VBR, 0)
                total_test = total_test + num_tests
                total_passed = total_passed + num_passed
        print ("---------------------------------------", file=open(test_name + '.txt', 'a'))
        return total_test, total_passed

## -------------- COMPARE TESTS -------------- ##
## These test checks for bitstream exactness, these comparisons are done to make sure specific features are working correctly
    def buffered_test(self, seq_list):
        # Test specific parameters:
        test_name = 'buffered_test'
        # Get default encoding params
        enc_params = self.get_default_params().copy()
        if os.path.exists(test_name + '.txt'):
            return 0, 0
        print ("Running Test: " + test_name)
        print ("---------------------------------------", file=open(test_name + '.txt', 'w'))
        print ("Test Name: " + test_name, file=open(test_name + '.txt', 'a'))
        total_test = 0
        total_passed = 0
        for seq in seq_list:
            test_params = [
            [seq, {'buffered_input': -1}],
            [seq, {'buffered_input': NUM_FRAMES}]
            ]
            # Run test
            for VBR in QP_VBR_COMBINATION:
                num_tests, num_passed = self.run_test(test_name, test_params, enc_params, VBR, not VBR)
                total_test = total_test + num_tests
                total_passed = total_passed + num_passed
        print ("---------------------------------------", file=open(test_name + '.txt', 'a'))
        return total_test, total_passed

    def run_to_run_test(self, seq_list):
        # Test specific parameters:
        test_name = 'run_to_run_test'
        # Get default encoding params
        enc_params = self.get_default_params().copy()
        if os.path.exists(test_name + '.txt'):
            return 0, 0
        print ("Running Test: " + test_name)
        print ("---------------------------------------", file=open(test_name + '.txt', 'w'))
        print ("Test Name: " + test_name, file=open(test_name + '.txt', 'a'))
        total_test = 0
        total_passed = 0
        for seq in seq_list:
            test_params = [
            [seq, {'frame_to_be_encoded': 150}],
            [seq, {'frame_to_be_encoded': 150}]
            ]
            # Run test
            num_tests, num_passed = self.run_test(test_name, test_params, enc_params, 0, 1)
            total_test = total_test + num_tests
            total_passed = total_passed + num_passed
        print ("---------------------------------------", file=open(test_name + '.txt', 'a'))
        return total_test, total_passed

    def unpacked_test(self, seq_list):
        # Test specific parameters:
        test_name = 'unpacked_test'
        # Get default encoding params
        enc_params = self.get_default_params().copy()
        if os.path.exists(test_name + '.txt'):
            return 0, 0
        print ("Running Test: " + test_name)
        print ("---------------------------------------", file=open(test_name + '.txt', 'w'))
        print ("Test Name: " + test_name, file=open(test_name + '.txt', 'a'))
        total_test = 0
        total_passed = 0
        for seq in seq_list:
            compressed_seq_name = seq + '_2bitspacked'
            test_params = [
            [seq, {'compressed_ten_bit_format': 0}],
            [compressed_seq_name, {'compressed_ten_bit_format': 1}]
            ]
            # Run test
            for VBR in QP_VBR_COMBINATION:
                num_tests, num_passed = self.run_test(test_name, test_params, enc_params, VBR, not VBR)
                total_test = total_test + num_tests
                total_passed = total_passed + num_passed
        print ("---------------------------------------", file=open(test_name + '.txt', 'a'))
        return total_test, total_passed

    def defield_test(self, seq_list):
        # Test specific parameters:
        test_name = 'defield_test'
        # Get default encoding params
        enc_params = self.get_default_params().copy()
        if os.path.exists(test_name + '.txt'):
            return 0, 0
        print ("Running Test: " + test_name)
        print ("---------------------------------------", file=open(test_name + '.txt', 'w'))
        print ("Test Name: " + test_name, file=open(test_name + '.txt', 'a'))
        total_test = 0
        total_passed = 0
        for seq in seq_list:
            stream_info = self.get_stream_info(seq)
            defield_seq_name = seq + '_fields'
            test_params = [
            [seq, {'interlaced_video': 1, 'deinterlace_input' : 1}],
            [defield_seq_name, {'interlaced_video': 1, 'deinterlace_input' : 0}]
            ]
            test_params_2 = [
            [seq, {'interlaced_video': 0, 'deinterlace_input' : 1}],
            [defield_seq_name, {'interlaced_video': 0, 'deinterlace_input' : 0}]
            ]
            # Run test
            for VBR in QP_VBR_COMBINATION:
                num_tests, num_passed = self.run_test(test_name, test_params, enc_params, VBR, not VBR)
                total_test = total_test + num_tests
                total_passed = total_passed + num_passed
                num_tests, num_passed = self.run_test(test_name, test_params_2, enc_params, VBR, not VBR)
                total_test = total_test + num_tests
                total_passed = total_passed + num_passed
        print ("---------------------------------------", file=open(test_name + '.txt', 'a'))
        return total_test, total_passed
## ------------------------------------------- ##

## ------------ FUNCTIONAL TESTS ------------- ##
# These tests put the encoder through various combinations to ensure the encoder does not crash
    def qp_file_test(self,seq_list):
        # Test specific parameters:
        test_name = 'qp_file_test'
        combination_test_params = { 'use_qp_file'       : [1]
                                  }
        # Run tests
        return self.run_functional_tests(seq_list, test_name, combination_test_params)

    def intra_period_test(self,seq_list):
        # Test specific parameters:
        test_name = 'intra_period_test'
        intra = []
        intra_min = -2
        intra_max = 250
        bin = (float(intra_max) - float(intra_min))/float(LAD_ITER)
        for count in range(LAD_ITER):
            intra.append(random.randint(intra_min + int(bin*count), intra_min + int(bin*(count+1))))
        combination_test_params = { 'intra_period'      : intra,
                                    'IntraRefreshType'  : [1, 2],
                                    'frame_to_be_encoded': [260]
                                  }
        # Run tests
        return self.run_functional_tests(seq_list, test_name, combination_test_params)

    def enc_struct_test(self,seq_list):
        # Test specific parameters:
        test_name = 'enc_struct_test'
        combination_test_params = { 'HierarchicalLevels'    : [0,1,2,3],
                                    'PredStructure'         : [0,1,2],
                                    'BaseLayerSwitchMode'   : [0,1],
                                  }
        # Run tests
        return self.run_functional_tests(seq_list, test_name, combination_test_params)

    def width_height_test(self,seq_list):
        # Test specific parameters:
        test_name = 'width_height_test'
        widths, heights = self.get_width_height()
        combination_test_params = { 'width'    : widths,
                                    'height'   : heights
                                  }
        # Run tests
        return self.run_functional_tests(seq_list, test_name, combination_test_params)

    def dlf_test(self,seq_list):
        # Test specific parameters:
        test_name = 'dlf_test'
        combination_test_params = { 'LoopFilterDisable'     : [0, 1],
                                  }
        # Run tests
        return self.run_functional_tests(seq_list, test_name, combination_test_params)

    def sao_test(self,seq_list):
        # Test specific parameters:
        test_name = 'sao_test'
        combination_test_params = { 'SAO'     : [0, 1],
                                  }
        # Run tests
        return self.run_functional_tests(seq_list, test_name, combination_test_params)

    def constrained_intra_test(self,seq_list):
        # Test specific parameters:
        test_name = 'constrained_intra_test'
        combination_test_params = { 'ConstrainedIntra'     : [0, 1],
                                  }
        # Run tests
        return self.run_functional_tests(seq_list, test_name, combination_test_params)

    def scene_change_test(self,seq_list):
        # Test specific parameters:
        test_name = 'scene_change_test'
        lad = []
        lad_min = 0
        lad_max = 250
        bin = (float(lad_max) - float(lad_min))/float(LAD_ITER)
        for count in range(LAD_ITER):
            lad.append(random.randint(lad_min + int(bin*count), lad_min + int(bin*(count+1))))
        combination_test_params = { 'SceneChangeDetection'  : [0, 1],
                                    'LookAheadDistance'     : lad,
                                    'frame_to_be_encoded'   : [260]
                                  }
        # Run tests
        return self.run_functional_tests(seq_list, test_name, combination_test_params)

    def me_hme_test(self,seq_list):
        # Test specific parameters:
        test_name = 'me_hme_test'
        sa_w = []
        sa_h = []
        sa_min = 1
        sa_max = 255
        bin = (float(sa_max) - float(sa_min))/float(SA_ITER)
        for count in range(LAD_ITER):
            sa_w.append(random.randint(sa_min + int(bin*count), sa_min + int(bin*(count+1))))
            sa_h.append(random.randint(sa_min + int(bin*count), sa_min + int(bin*(count+1))))
        combination_test_params = { 'DefaultMeHme'  : [0],
                                    'HME'  : [1],
                                    'SearchAreaWidth'  : sa_w,
                                    'SearchAreaHeight'  : sa_h,
                                  }
        # Run tests
        return self.run_functional_tests(seq_list, test_name, combination_test_params)
## ------------------------------------------- ##

## --------------- DECODE TEST --------------- ##
# The decode test ensure encoded streams are conformant to standard decoders
    def decode_test(self,seq_list):
        # Test specific parameters:
        test_name = 'decode_test'
        # Get default encoding params
        enc_params = self.get_default_params().copy()
        if os.path.exists(test_name + '.txt'):
            return 0, 0
        print ("Running Test: " + test_name)
        print ("---------------------------------------", file=open(test_name + '.txt', 'w'))
        print ("Test Name: " + test_name, file=open(test_name + '.txt', 'a'))
        total_test = 0
        total_passed = 0
        for seq in seq_list:
            for VBR in QP_VBR_COMBINATION:
                test_params = [
                [seq, {'frame_to_be_encoded': 20}],
                ]
                num_tests, num_passed = self.run_test(test_name, test_params, enc_params, VBR, 0)
                total_test = total_test + num_tests
                total_passed = total_passed + num_passed
        return total_test, total_passed


## ------------------------------------------- ##

    def get_time(self, total_seconds):
        hours = int(total_seconds/3600)
        minutes = int((total_seconds - hours*3600)/60)
        seconds = int(total_seconds - minutes*60 - hours*3600)

        days = int(hours/24)
        hours = hours - days*24

        time_str = ""
        if days != 0:
            time_str = str(days) + " day(s), "
        if hours != 0:
            time_str = time_str + str(hours) + " hour(s), "
        if minutes != 0:
            time_str = time_str + str(minutes) + " minute(s), "
        if minutes != 0:
            time_str = time_str + str(seconds) + " second(s)"

        return time_str

    def error_check(self, seq_list):
        exit_code = 0
        if not os.path.exists(self.encoder_path + slash + exe_name):
            print ("Cannot find encoder executable. Please make sure " + exe_name + " can be found in the folder \"" + self.encoder_path + "\"")
            exit_code = -1
            return exit_code
        if not os.path.exists(self.tools_path + slash + dec_exe):
            print ("Cannot find decoder executable. Please make sure " + dec_exe + " can be found in the folder \"" + self.tools_path + "\"")
            exit_code = -1
            return exit_code
        for seq in seq_list:
            if not os.path.exists(self.yuv_path + slash + seq + '.yuv'):
                print ("Cannot find " + seq + ".yuv in yuv_path")
                exit_code = -2
            if TEST_CONFIGURATION == 0:
                if "_8bit_" in seq:
                    if not os.path.exists(self.yuv_path + slash + seq + '_fields' + '.yuv'):
                        print ("Cannot find " + seq + "_fields.yuv in yuv_path")
                        exit_code = -2
                elif "_10bit_" in seq:
                    if not os.path.exists(self.yuv_path + slash + seq + '_2bitspacked' + '.yuv'):
                        print ("Cannot find " + seq + "_2bitspacked.yuv in yuv_path")
                        exit_code = -2
        if exit_code == -2 and TEST_CONFIGURATION == 0:
            print ("8 bits sequences need to have the fields-separated counterpart")
            print ("10 bits sequences need to have the 2bitspacked counterpart")
        return exit_code

    def run_validation_test(self, seq_list):
        if self.error_check(seq_list) != 0:
            return
        file_name = "Test_Results"
        print ("---------------------------------------------------------", file=open(file_name + '.txt', 'w'))
        print ("Test Begin... ", file=open(file_name + '.txt', 'a'))
        start_time = time.time()
        total_tests = 0
        total_passed = 0
        num_tests, num_passed = self.defield_test(seq_list)
        total_tests = total_tests + num_tests
        total_passed = total_passed + num_passed
        num_tests, num_passed = self.intra_period_test(seq_list)
        total_tests = total_tests + num_tests
        total_passed = total_passed + num_passed
        num_tests, num_passed = self.width_height_test(seq_list)
        total_tests = total_tests + num_tests
        total_passed = total_passed + num_passed
        num_tests, num_passed = self.buffered_test(seq_list)
        total_tests = total_tests + num_tests
        total_passed = total_passed + num_passed
        num_tests, num_passed = self.run_to_run_test(seq_list)
        total_tests = total_tests + num_tests
        total_passed = total_passed + num_passed
        num_tests, num_passed = self.qp_file_test(seq_list)
        total_tests = total_tests + num_tests
        total_passed = total_passed + num_passed
        num_tests, num_passed = self.enc_struct_test(seq_list)
        total_tests = total_tests + num_tests
        total_passed = total_passed + num_passed
        num_tests, num_passed = self.unpacked_test(seq_list)
        total_tests = total_tests + num_tests
        total_passed = total_passed + num_passed
        num_tests, num_passed = self.dlf_test(seq_list)
        total_tests = total_tests + num_tests
        total_passed = total_passed + num_passed
        num_tests, num_passed = self.sao_test(seq_list)
        total_tests = total_tests + num_tests
        total_passed = total_passed + num_passed
        num_tests, num_passed = self.constrained_intra_test(seq_list)
        total_tests = total_tests + num_tests
        total_passed = total_passed + num_passed
        num_tests, num_passed = self.scene_change_test(seq_list)
        total_tests = total_tests + num_tests
        total_passed = total_passed + num_passed
        num_tests, num_passed = self.me_hme_test(seq_list)
        total_tests = total_tests + num_tests
        total_passed = total_passed + num_passed
        num_tests, num_passed = self.decode_test(seq_list)
        total_tests = total_tests + num_tests
        total_passed = total_passed + num_passed
        finish_time = time.time()
        if total_tests == 0 and total_passed == 0:
            print ("No tests were ran.. Exiting...", file=open(file_name + '.txt', 'a'))
            print ("---------------------------------------------------------", file=open(file_name + '.txt', 'a'))
            return
        print ("Total Number of Tests: " + str(total_tests), file=open(file_name + '.txt', 'a'))
        print ("Total Passed: " + str(total_passed), file=open(file_name + '.txt', 'a'))
        print ("Percentage Passed: " + str(float(total_passed)/float(total_tests)*100) + "%", file=open(file_name + '.txt', 'a'))
        print ("Time Elapsed: " + self.get_time(finish_time - start_time), file=open(file_name + '.txt', 'a'))
        print ("---------------------------------------------------------", file=open(file_name + '.txt', 'a'))

    def show_speed_test_instructions(self):
        print ("To run speed test:", file=open('Running_Speed_Test.txt', 'w'))
        print ("1. Run shell script (Run in \"sudo\" mode in Linux)", file=open('Running_Speed_Test.txt', 'a'))
        print ("Note: Running it from a shell script minimizes use of CPU cycles from Python", file=open('Running_Speed_Test.txt', 'a'))

    # Set number of channels for speed test
    def get_num_channels(self, enc_mode, enc_params):
        if enc_params['width']*enc_params['height'] > 1920*1080:
            if enc_mode >= 0 and enc_mode <= 3:
                num_channels = 1
            elif enc_mode >= 4 and enc_mode <= 7:
                num_channels = 1
            elif enc_mode >= 8:
                num_channels = 2
        elif enc_params['width']*enc_params['height'] > 1280*720:
            if enc_mode >= 0 and enc_mode <= 3:
                num_channels = 2
            elif enc_mode >= 4 and enc_mode <= 7:
                num_channels = 2
            elif enc_mode >= 8:
                num_channels = 4
        elif enc_params['width']*enc_params['height'] > 864*480:
            if enc_mode >= 0 and enc_mode <= 3:
                num_channels = 4
            elif enc_mode >= 4 and enc_mode <= 7:
                num_channels = 4
            elif enc_mode >= 8:
                num_channels = 6
        else:
            num_channels = 6
        return num_channels

    # Set number of frames for speed test
    def get_num_frames(self, enc_mode):
        if enc_mode >= 0 and enc_mode <= 1:
            num_frames = 500
        elif enc_mode >= 2 and enc_mode <= 4:
            num_frames = 2000
        elif enc_mode >= 5:
            num_frames = 4000
        return num_frames

    def run_speed_test(self, seq_dict):
        seq_list = []
        for x in seq_dict:
            seq_list.append(x['name'])
        if self.error_check(seq_list) != 0:
            return
        print("---------------------------------------------------------")
        print("Speed Test")
        print("To run speed test, read \"Running_Speed_Test.txt\"")
        print("---------------------------------------------------------\n")
        self.show_speed_test_instructions()
        enc_params = self.get_default_params().copy()
        if platform == WINDOWS_PLATFORM_STR:
            print("", file=open('speed_script.bat', 'w'))
        else:
            print("", file=open('speed_script.bat', 'w'))
        for seq in seq_dict:
            enc_params.update(self.get_stream_info(seq['name']))
            if 'qp' in seq and 'tbr' in seq:
                print("Please only have \"qp\" or \"tbr\" for each sequence")
                continue
            elif not 'qp' in seq and not 'tbr' in seq:
                print("Please have \"qp\" or \"tbr\" for each sequence")
                continue
            elif 'qp' in seq:
                VBR = 0
                iter_list = seq['qp']
            elif 'tbr' in seq:
                VBR = 1
                iter_list = seq['tbr']
            if VBR == 1:
                continue
            for enc_mode in SPEED_ENC_MODES:
                num_channels = self.get_num_channels(enc_mode, enc_params)
                num_frames = self.get_num_frames(enc_mode)
                if VBR == 0:
                    enc_params.update({'enc_mode': enc_mode, 'qp': iter_list, 'rc': 0, 'frame_to_be_encoded': num_frames})
                    cmd = self.get_enc_cmd(enc_params, seq['name'], 'Speed_Test_M' + str(enc_mode) + '_' + seq['name'] + '_Q' + str(iter_list), num_channels)
                else:
                    enc_params.update({'enc_mode': enc_mode, 'tbr': iter_list, 'rc': 1, 'frame_to_be_encoded': num_frames})
                    cmd = self.get_enc_cmd(enc_params, seq['name'], 'Speed_Test_M' + str(enc_mode) + '_' + seq['name'] + '_TBR' + str(iter_list), num_channels)
                print (cmd)
                if platform == WINDOWS_PLATFORM_STR:
                    print(cmd, file=open('speed_script.bat', 'a'))
                else:
                    print(cmd, file=open('speed_script.sh', 'a'))

##----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------##
small_test = EB_Test(ENC_PATH, BIN_PATH, YUV_PATH, TOOLS_PATH)
if TEST_CONFIGURATION == 0:
    small_test.run_validation_test(VALIDATION_TEST_SEQUENCES)
elif TEST_CONFIGURATION == 1:
    small_test.run_speed_test(SPEED_TEST_SEQUENCES)


