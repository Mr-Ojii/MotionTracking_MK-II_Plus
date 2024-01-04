# MotionTracking_MKII_Plus
AviUtl object tracking (rubbish) plugin based on LKT/optical flow. Use OpenCV 4.5.1 or newer.


## System Requirement
*- CPU with AVX support (Not tested on SSE-only CPU)  
*- \>= 2GB RAM  
*- Windows 7 or later  
*- AviUtl 1.10  
*- Adv.Editing 0.92


## Installation
Dump the .AUF and .XML files into  
`AviUtl's root folder`

The menu name should be "MotionTracking MK-II Plus"

Additional work is required if Methods 4, 5, or 6 are to be used.

### For Method 4 (GOTURN)
1. Download each data from [this URL](https://github.com/opencv/opencv_extra/tree/c4219d5eb3105ed8e634278fad312a1a8d2c182d/testdata/tracking) 
2. Extract the splited ZIP file
3. Dump the
    - goturn.caffemodel
    - goturn.prototxt  
  into `AviUtl's root folder`

### For Method 5 (DaSiamRPN)
1. Download the following file from the URL listed as a comment out in the source code at [this URL](https://github.com/opencv/opencv/blob/4.x/samples/dnn/dasiamrpn_tracker.cpp)

- dasiamrpn_model.onnx
- dasiamrpn_kernel_r1.onnx
- dasiamrpn_kernel_cls1.onnx

2. Dump each files into `AviUtl's root folder`

### For Method 6 (Nano)
1. Download the following file from [this URL](https://github.com/HonglinChu/SiamTrackers/tree/211ff3f862fc68a870dde1ab00451f35af3b1d4/NanoTrack/models/nanotrackv2)

- nanotrack_backbone.onnx
- nanotrack_head.onnx

2. Dump each files into `AviUtl's root folder`

### For Method 7 (Vit)
1. Download the following file from [this URL](https://github.com/opencv/opencv_extra/blob/4.x/testdata/dnn/onnx/models/vitTracker.onnx)

- vitTracker.onnx

2. Dump each files into `AviUtl's root folder`

## Helper Plugins
The single AUF actually contains two more helper plugins:
1. Pre-track: HSV Cvt
2. Pre-track: BGSubtraction

HSVCvt convert the RGB image into HSV, then display it as if RGB. It can also display only one of the HSV channels.

BGSubtraction aims to isolate the moving object from the background. It can output the isolated RGB image, or output a grey-scale mask. Beware that a large Range value may cause out-of-memory problem, or enable Large-Address-Aware to get around.


## Help
### MotionTracking MK-II Plus
#### Method
1. Multi Instance Learning
2. KCF
3. CSRT
4. GOTURN
5. DaSiamRPN
6. Nano
7. Vit
#### Steps
0. Mark a section to track
1. Click 1st button, Drag a box on the object to be tracked(in popup Window). Close the popup Window.
2. Click Analyze, wait for completion.
3. Activate the View Result and check. IF result is good, click SaveEXO or check QuickBlur. Otherwise, click Clear Result and go back to step 0 or 1.
#### Save EXO
Auto correct for single sandwiched error result.  
Support CJK filename
#### Options
- Quick Blur : Direct blur on AviUtl Window according to tracking result.
- Easy Privacy : Blur all detected faces(real face only), No tracking is needed. Works well on frontal face, poor on profile face.

### Pre-track:BGSubtraction
#### Common Parameters
- Range : Use <Range> no. of frames before and after current frame for analysis.[30]
- Shadow : 1= Extract shadow [0]
#### MOG2-Only
- NMix : Number of Gaussian mixtures [5]
- BG% : Background ratio [70%] 
#### KNN-Only
- d2T : Threshold on the squared distance between the pixel and the sample to decide whether a pixel is close to that sample.


## Building From Source
1. Run `git clone https://github.com/Mr-Ojii/MotionTracking_MKII_Plus.git`
2. Build your own OpenCV 4.9.0 STATIC (Both Debug and Release builds)
3. Generate VS project file from src/CMakeLists.txt
4. Build with Visual Studio

## Bug Report
* [GitHub](https://github.com/Mr-Ojii/MotionTracking_MKII_Plus)
