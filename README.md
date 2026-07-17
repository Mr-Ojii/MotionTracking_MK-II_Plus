# MotionTracking_MK-II_Plus_for_AviUtl2

AviUtl ExEdit2 object tracking (rubbish) plugin based on LKT/optical flow.

## System Requirement

- CPU with AVX2 support
- Windows 10 or later
- DirectX11.3 support
- AviUtl ExEdit2 version 2.0beta48 or later
- Tested with AviUtl ExEdit2 version 2.1.0

## Installation

Dump the .aux2 and MotionTracking_model folder into  
`your favorite folder where AviUtl loads aux2 file`

The menu name should be "MotionTracking MK-II Plus for AviUtl2"

Additional work is required if DaSiamRPN, Nano or Vit are to be used.

### DaSiamRPN

1. Download the following file from the URL listed as a comment out in the source code at [this URL](https://github.com/opencv/opencv/blob/4.x/samples/dnn/dasiamrpn_tracker.cpp)

- dasiamrpn_model.onnx
- dasiamrpn_kernel_r1.onnx
- dasiamrpn_kernel_cls1.onnx

2. Dump each files into `MotionTracking_model folder`

### Nano

1. Download the following file from [this URL](https://github.com/HonglinChu/SiamTrackers/tree/18b7791360acb3f6d276d47376a6f1ed516f1628/NanoTrack/models/nanotrackv2)

- nanotrack_backbone_sim.onnx
- nanotrack_head_sim.onnx

2. Dump each files into `MotionTracking_model folder`

### Vit

1. Download the following file from [this URL](https://github.com/opencv/opencv_extra/blob/4.x/testdata/dnn/onnx/models/vitTracker.onnx)

- vitTracker.onnx

2. Dump each files into `MotionTracking_model folder`

## Helper Plugins

> [!Important]
> At this stage, Helper Plugins are not supported. I plan to add support in a future update.

The single AUX2 actually contains two more helper plugins:

1. Pre-track: HSV Cvt
2. Pre-track: BGSubtraction

HSVCvt convert the RGB image into HSV, then display it as if RGB. It can also display only one of the HSV channels.

BGSubtraction aims to isolate the moving object from the background. It can output the isolated RGB image, or output a grey-scale mask. Beware that a large Range value may cause out-of-memory problem, or enable Large-Address-Aware to get around.

## Help

### MotionTracking MK-II Plus

#### Steps

0. Mark a section to track
1. Click 1st button, Drag a box on the object to be tracked(in popup Window). Close the popup Window by clicking the x button or pressing F3.
2. Click Analyze, wait for completion. To cancel the analysis, click the X button.
3. Activate the View Result and check. If result is good, check Invert Position if necessary, click Insert Object or save Object file. Otherwise, click Clear Result and go back to step 0 or 1.

#### Export Object File

Auto correct for single sandwiched error result.  
Support CJK filename

#### Options

#### Dropdown options

##### Method

Specifies the algorithm to be used in the analysis.

1. Multi Instance Learning
2. KCF
3. CSRT
4. DaSiamRPN
5. Nano
6. Vit

##### Hue

Specifies the hue of the rectangle displayed in Object Selection and View Result.

##### Insert Object Options

- As Sub-filter/部分フィルタ？ : Output as a sub filter.
- Invert Position : Reverse the position of the tracking result.
- Ignore Aspect Ratio : Ignore the aspect ratio and output in scale.

### Pre-track:BGSubtraction

> [!Important]
> At this stage, Pre-track:BGSubtraction is not supported. I plan to add support in a future update.

#### Common Parameters

- Range : Use <Range> no. of frames before and after current frame for analysis.[30]
- Shadow : 1= Extract shadow [0]

#### MOG2-Only

- NMix : Number of Gaussian mixtures [5]
- BG% : Background ratio [70%]

#### KNN-Only

- d2T : Threshold on the squared distance between the pixel and the sample to decide whether a pixel is close to that sample.

## Building From Source

Please read `.github/workflows/build.yml` or [Dockerfile](https://github.com/nullruptr/MotionTracking_MK-II_Plus_for_AviUtl2/tree/master/docker).

## Bug Report

- [GitHub](https://github.com/nullruptr/MotionTracking_MK-II_Plus_for_AviUtl2)
