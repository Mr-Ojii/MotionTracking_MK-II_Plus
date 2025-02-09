# MotionTracking MK-II Plus
AviUtl object tracking (rubbish) plugin based on LKT/optical flow. Use OpenCV 4.9.0 or newer.


## System Requirement
*- CPU with AVX support (Not tested on SSE-only CPU)  
*- \>= 2GB RAM  
*- Windows 7 or later  
*- AviUtl 1.10  
*- Adv.Editing 0.92


## Installation
Dump the .AUF and MotionTracking_model folder into  
`your favorite folder where AviUtl loads filter plug-ins`

The menu name should be "MotionTracking MK-II Plus"

Additional work is required if Methods 4, 5 or 6 are to be used.

### For Method 4 (DaSiamRPN)
1. Download the following file from the URL listed as a comment out in the source code at [this URL](https://github.com/opencv/opencv/blob/4.x/samples/dnn/dasiamrpn_tracker.cpp)

- dasiamrpn_model.onnx
- dasiamrpn_kernel_r1.onnx
- dasiamrpn_kernel_cls1.onnx

2. Dump each files into `MotionTracking_model folder`

### For Method 5 (Nano)
1. Download the following file from [this URL](https://github.com/HonglinChu/SiamTrackers/tree/18b7791360acb3f6d276d47376a6f1ed516f1628/NanoTrack/models/nanotrackv2)

- nanotrack_backbone_sim.onnx
- nanotrack_head_sim.onnx

2. Dump each files into `MotionTracking_model folder`

### For Method 6 (Vit)
1. Download the following file from [this URL](https://github.com/opencv/opencv_extra/blob/4.x/testdata/dnn/onnx/models/vitTracker.onnx)

- vitTracker.onnx

2. Dump each files into `MotionTracking_model folder`

## Helper Plugins
The single AUF actually contains two more helper plugins:
1. Pre-track: HSV Cvt
2. Pre-track: BGSubtraction

HSVCvt convert the RGB image into HSV, then display it as if RGB. It can also display only one of the HSV channels.

BGSubtraction aims to isolate the moving object from the background. It can output the isolated RGB image, or output a grey-scale mask. Beware that a large Range value may cause out-of-memory problem, or enable Large-Address-Aware to get around.


## Help
### MotionTracking MK-II Plus
#### Steps
0. Mark a section to track
1. Click 1st button, Drag a box on the object to be tracked(in popup Window). Close the popup Window.
2. Click Analyze, wait for completion.
3. Activate the View Result and check. IF result is good, check Invert Position if necessary, click SaveEXO or check QuickBlur. Otherwise, click Clear Result and go back to step 0 or 1.
#### Save EXO
Auto correct for single sandwiched error result.  
Support CJK filename
#### Options
##### Method
Specifies the algorithm to be used in the analysis.
1. Multi Instance Learning
2. KCF
3. CSRT
4. DaSiamRPN
5. Nano
6. Vit
##### Rect Hue
Specifies the hue of the rectangle displayed in Object Selection and View Result.
##### Save EXO Options
- As Sub-filter/部分フィルター？ : Output as a sub filter.
- Invert Position : Reverse the position of the tracking result.
###### Note
“As English EXO?” option has been removed. If you want to load .exo into the English version of AviUtl, please use [AviUtl-Exo-Language-Converter](https://github.com/Mr-Ojii/AviUtl-Exo-Language-Converter) or [GCMZDrops](https://github.com/oov/aviutl_gcmzdrops) instead.  
##### Video Effect Options
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
1. Run `git clone https://github.com/Mr-Ojii/MotionTracking_MK-II_Plus.git`
2. Build your own OpenCV 4.9.0 STATIC (Both Debug and Release builds)
3. Generate VS project file from src/CMakeLists.txt
4. Build with Visual Studio

## Bug Report
* [GitHub](https://github.com/Mr-Ojii/MotionTracking_MK-II_Plus)
