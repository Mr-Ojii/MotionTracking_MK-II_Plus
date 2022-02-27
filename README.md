# MotionTracking_MKII_Plus
AviUtl object tracking (rubbish) plugin based on LKT/optical flow. Use OpenCV 4.5.1 or newer.

## System Requirement
*- Intel Core-i series processor  
OR  
*- CPU with AVX support (Not tested on SSE-only CPU)  
*- \>= 2GB RAM  
*- Windows 7 or later  
*- AviUtl 1.10 and Adv.Editing 0.92

## Installation
Dump the .AUF and .XML files into either  
`AviUtl's root folder`  
or  
`the ./Plugins sub-folder`

The menu name should be "MotionTracking MK-II Plus"

## Helper Plugins
The single AUF actually contains two more helper plugins:
1. Pre-track: HSV Cvt
2. Pre-track: BGSubtraction

HSVCvt convert the RGB image into HSV, then display it as if RGB. It can also display only one of the HSV channels.

BGSubtraction aims to isolate the moving object from the background. It can output the isolated RGB image, or output a grey-scale mask. Beware that a large Range value may cause out-of-memory problem, or enable Large-Address-Aware to get around.

## Help
1. Load some video/New project
2. Activate the plugin
3. Click on the "HELP" button

## Building From Source
1. Run `git clone https://github.com/Mr-Ojii/MotionTracking_MKII_Plus.git`
2. Build your own OpenCV 4.5.5 STATIC (Both Debug and Release builds)
3. Generate VS project file from src/CMakeLists.txt
4. Build with Visual Studio

## Bug Report
* [GitHub](https://github.com/Mr-Ojii/MotionTracking_MKII_Plus)