== Motion Tracking MK-II Plugin for AviUtl ==
by Maverick Tse, 2014
=============================================
DL: http://1drv.ms/1wRfp6G

=== Introduction ===
  This should be a huge improvement over the
previous MK-I plugin in terms of UI design
and tracking robustness and performance.

=== System Requirement ===
*- Intel Core-i series processor
OR
*- CPU with AVX support (Not tested on SSE-only CPU)
*- >= 2GB RAM
*- Windows 7 or later
*- VC2013 redistributable
*- AviUtl 1.00 and Adv.Editing 0.92

=== Installation ===
Dump the .AUF and .XML files into either
*- AviUtl's root folder
OR
*- the ./Plugins sub-folder

The menu name should be "MotionTracking MK-II"

=== Helper Plugins ===
The single AUF actually contains two more
helper plugins:
1. Pre-track: HSV Cvt
2. Pre-track: BGSubtraction

HSVCvt convert the RGB image into HSV, then display it
as if RGB. It can also display only one of the HSV channels.

BGSubtraction aims to isolate the moving object from the
background. It can output the isolated RGB image, or output
a grey-scale mask. Beware that a large Range value may cause
out-of-memory problem, or enable Large-Address-Aware to get
around.

=== Help ===
*- Load some video/New project
*- Activate the plugin
*- Click on the "HELP" button

=== Building From Source ===
*- Decompress "Src.7z"
*- Build your own OpenCV 3.0 STATIC
(Both Debug and Release builds)
*- Open the solution with VC2013
*- Correct the include/lib paths
*- Customize the "Debug" settings
*- Press F7 to build

=== Bug Report ===
*- Twitter @MaverickTse
*- videohelp.com : Editing> AviUtl Support Thread