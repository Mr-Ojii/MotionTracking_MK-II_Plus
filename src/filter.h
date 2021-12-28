#pragma once
#include <Windows.h>
//----------------------------------------------------------------------------------
//	Filter Plugin Header file for AviUtl version 0.99k and later
//	By Mr. Ken, Comment translated to English by Maverick Tse
//  Translation completed on 2014-05-05
//----------------------------------------------------------------------------------

//	YC Struct
typedef	struct {
	short	y;					//	Pixel data-Y (     0 Å` 4096 )
	short	cb;					//	Pixel data-Cb ( -2048 Å` 2048 )
	short	cr;					//	Pixel data-Cr ( -2048 Å` 2048 )
	//	Pixel data can get outside of the above range
	//	
} PIXEL_YC;

//	PIXEL Struct
typedef	struct {
	unsigned char	b, g, r;		//	Pixel(RGB)Data (0Å`255)
} PIXEL;

//	Filter_PROC Struct
typedef struct {
	int			flag;			//	Filter flag
	//	FILTER_PROC_INFO_FLAG_INVERT_FIELD_ORDER	: Reverse field order ( normally treat as BFF )
	//	FILTER_PROC_INFO_FLAG_INVERT_INTERLACE		: invert deinterlace method (For use with deinterlace filter only)
	PIXEL_YC	*ycp_edit;		//	Pointer to image data ( ycp_editand ycp_temp can be swapped )
	PIXEL_YC	*ycp_temp;		//	pointer to temp area
	int			w, h;			//	image/frame size ( image size can be changed )
	int			max_w, max_h;	//	Canvas size (set in ENVIRONMENT SETTINGS)
	int			frame;			//	Current frame no.( starts from zero )
	int			frame_n;		//	Total frame count
	int			org_w, org_h;	//	Original image size
	short		*audiop;		//	Pointer to audio data ( For audio file only )
	//	Audio format is PCM16bit ( for one sample, mono = 2byte , stereo = 4byte )
	int			audio_n;		//	Total audio sample
	int			audio_ch;		//	No. of audio channel
	PIXEL		*pixelp;		//	Not used
	void		*editp;			//	Edit handle
	int			yc_size;		//	Size of total pixel data in bytes
	int			line_size;		//	Width of image area in bytes
	int			reserve[8];		//	Reserved for future use
} FILTER_PROC_INFO;
#define	FILTER_PROC_INFO_FLAG_INVERT_FIELD_ORDER	0x00010000
#define	FILTER_PROC_INFO_FLAG_INVERT_INTERLACE		0x00020000
//	Å¶For de-interlace filter, ycp_edit is not initialized with image dataÅB
//	Å¶De-interlace filter cannot change ycp_edit,ycp_temp,w and h

//	Frame Status Struct
typedef struct {
	int		video;			//	Video data ID
	int		audio;			//	Audio data ID
	int		inter;			//	Interlace status
	//	FRAME_STATUS_INTER_NORMAL	: Normal
	//	FRAME_STATUS_INTER_REVERSE	: Reverse
	//	FRAME_STATUS_INTER_ODD		: Odd
	//	FRAME_STATUS_INTER_EVEN		: Even
	//	FRAME_STATUS_INTER_MIX		: BOB
	//	FRAME_STATUS_INTER_AUTO		: Auto
	int		index24fps;		//	Not used
	int		config;			//	Profile ID for frame
	int		vcm;			//	Compression setting ID for frame
	int		edit_flag;		//	Editing flag
	//	EDIT_FRAME_EDIT_FLAG_KEYFRAME	: Keyframe
	//	EDIT_FRAME_EDIT_FLAG_MARKFRAME	: Marked frame
	//	EDIT_FRAME_EDIT_FLAG_DELFRAME	: preferred frame for decimation
	//	EDIT_FRAME_EDIT_FLAG_NULLFRAME	: Copy frame
	int		reserve[9];		//	Reserved for future expansion
} FRAME_STATUS;
#define	FRAME_STATUS_INTER_NORMAL		0
#define	FRAME_STATUS_INTER_REVERSE		1
#define	FRAME_STATUS_INTER_ODD			2
#define	FRAME_STATUS_INTER_EVEN			3
#define	FRAME_STATUS_INTER_MIX			4
#define	FRAME_STATUS_INTER_AUTO			5
#define	EDIT_FRAME_EDIT_FLAG_KEYFRAME		1
#define	EDIT_FRAME_EDIT_FLAG_MARKFRAME		2
#define	EDIT_FRAME_EDIT_FLAG_DELFRAME		4
#define	EDIT_FRAME_EDIT_FLAG_NULLFRAME		8

//	File info Struct
typedef struct {
	int		flag;					//	File flag
	//	FILE_INFO_FLAG_VIDEO	: Video exists
	//	FILE_INFO_FLAG_AUDIO	: Audio exists
	LPSTR	name;					//	File name ( avi_file_open() will give NULL )
	int		w, h;					//	Original size
	int		video_rate, video_scale;	//	Frame rate
	int		audio_rate;				//	Audio sampling rate
	int		audio_ch;				//	Audio channel count
	int		frame_n;				//	Total frame count
	DWORD	video_decode_format;	//	Video decode format
	int		video_decode_bit;		//	bit count for video decode
	int		audio_n;				//	Total sample count for audio ( only set when calling avi_file_open())
	int		reserve[4];				//	Reserved for future use
} FILE_INFO;
#define FILE_INFO_FLAG_VIDEO	1
#define FILE_INFO_FLAG_AUDIO	2

//	System Info Struct
typedef struct {
	int		flag;					//	System flag
	//	SYS_INFO_FLAG_EDIT		: Editing
	//	SYS_INFO_FLAG_VFAPI		: VFAPI is running
	//	SYS_INFO_FLAG_USE_SSE	: using SSE
	//	SYS_INFO_FLAG_USE_SSE2	: using SSE2
	LPSTR	info;					//	Version info
	int		filter_n;				//	No. of registered filters
	int		min_w, min_h;			//	Min. editable image size
	int		max_w, max_h;			//	Max. editable image size
	int		max_frame;				//	Max. frame count that can be handled
	LPSTR	edit_name;				//	Editing file's name (Contains nothing if filename is not set)
	LPSTR	project_name;			//	Project file's name (Contains nothing if filename is not set)
	LPSTR	output_name;			//	Output file name (Contains nothing if filename is not set)
	int		vram_w, vram_h;			//	Size of image data for editing
	int		vram_yc_size;			//	Size of total pixel data in bytes
	int		vram_line_size;			//	Width of image data in bytes
	HFONT	hfont;					//	Font handle used by filter's setting dialog
	int		build;					//	Build No. (Increase with new version)
	int		reserve[2];				//	Reserved for future use
} SYS_INFO;
#define SYS_INFO_FLAG_EDIT		1
#define SYS_INFO_FLAG_VFAPI		2
#define SYS_INFO_FLAG_USE_SSE	4
#define SYS_INFO_FLAG_USE_SSE2	8

//	Declaration for Multi-thread function
//	thread_id	: Thread ID ( 0 ~ thread_num-1 )
//	thread_num	: Thread count ( 1 ~ )
//	param1		: general parameter
//	param2		: general parameter
typedef void(*MULTI_THREAD_FUNC)(int thread_id, int thread_num, void *param1, void *param2);

//	Handle for Inputted AVI file
typedef void*	AVI_FILE_HANDLE;

//	Struct for external functions
typedef struct {
	//	Å¶Use of get_ycp_source_cache() is preferred
	//
	// Obtains Image Data's pointer of Frame plus offset
	//	The data are those before any filtering
	//	editp 	: Editing Handle
	//	n	 	: Frame number
	//  ofs	 	: Offset from Frame
	//  Return Value	: pointer to Image Data (Return NULL when Failed)
	//			  Content of Image Data pointer is valid until external function or Main returns
	void(*get_ycp_ofs)(void *editp, int n, int ofs);

	//	Å¶Use of get_ycp_source_cache() is preferred
	// Obtains Image Data's pointer of specified Frame
	//	The data are those before any filtering
	//	editp 	: Editing Handle
	//	n	 	: Frame number
	//  Return Value	: pointer to Image Data (Return NULL when Failed)
	//			  Content of Image Data pointer is valid until external function or Main returns
	void		*(*get_ycp)(void *editp, int n);

	// Obtains Image Data's pointer for specified Frame in DIB format(RGB24bit)
	//	The data are those before any filtering
	//	editp 	: Editing Handle
	//	n		: Frame number
	//  Return Value	: pointer to DIB-format data (Return NULL when Failed)
	//			  Content of Image Data pointer is valid until external function or Main returns
	void		*(*get_pixelp)(void *editp, int n);

	//	Read Audio Data of specified Frame
	//	The data are those before any filtering
	//	editp 	: Editing Handle
	//	n		: Frame number
	//	buf 	: Storage buffer (NULL to get sample count only)
	//  Return Value	: Imported sample count
	int(*get_audio)(void *editp, int n, void *buf);

	//	Check if editing
	//	editp 	: Editing Handle
	//  Return Value	: TRUE if editing
	BOOL(*is_editing)(void *editp);

	//	Check if saving
	//	editp 	: Editing Handle
	//  Return Value	: TRUE if saving
	BOOL(*is_saving)(void *editp);

	// Obtains Currently Displaying Frame
	//	editp 	: Editing Handle
	//  Return Value	: Current Frame number
	int(*get_frame)(void *editp);

	// Obtains total frame count
	//	editp 	: Editing Handle
	//  Return Value	: Current total frame count
	int(*get_frame_n)(void *editp);

	// Obtains Frame-size before filtering
	//	editp 	: Editing Handle
	//	w,h 	: pointers for storing image size
	//  Return Value	: TRUE if successful
	BOOL(*get_frame_size)(void *editp, int *w, int *h);

	// Change Currently Displaying frame
	//	editp 	: Editing Handle
	//  n		: Frame number
	//  Return Value	: Set Frame number
	int(*set_frame)(void *editp, int n);

	// Change total frame count
	//	editp 	: Editing Handle
	//  n		: Frame count
	//  Return Value	: Set total frame count
	int(*set_frame_n)(void *editp, int n);

	//	Copy frame from one to another
	//	editp 	: Editing Handle
	//	d	 	: Frame number of copy destination
	//	s	 	: Frame number of copy source
	//  Return Value	: TRUE if successful
	BOOL(*copy_frame)(void *editp, int d, int s);

	//	Copy ONLY image data to other frame
	//	editp 	: Editing Handle
	//	d	 	: Frame number of copy destination
	//	s	 	: Frame number of copy source
	//  Return Value	: TRUE if successful
	BOOL(*copy_video)(void *editp, int d, int s);

	//	Copy ONLY audio data to other frame
	//	editp 	: Editing Handle
	//	d	 	: Frame number of copy destination
	//	s	 	: Frame number of copy source
	//  Return Value	: TRUE if successful
	BOOL(*copy_audio)(void *editp, int d, int s);

	//	Copy DIB image(RGB24bit) to clipboard
	//	hwnd 	: Window handle
	//	pixelp	Pointer to DIB-format data
	//	w,h 	: Image size
	//  Return Value	: TRUE if successful
	BOOL(*copy_clip)(HWND hwnd, void *pixelp, int w, int h);

	// Paste image from Clipboard
	//	hwnd 	: Window Handle
	//	editp 	: Editing Handle
	//  n		: Frame number
	//  Return Value	: TRUE if successful
	BOOL(*paste_clip)(HWND hwnd, void *editp, int n);

	// Obtains Frame status
	//	editp 	: Editing Handle
	//  n		: Frame number
	//  fps		Pointer to Frame status
	//  Return Value	: TRUE if successful
	BOOL(*get_frame_status)(void *editp, int n, FRAME_STATUS *fsp);

	// Change Frame status
	//	editp 	: Editing Handle
	//  n		: Frame number
	//  fsp		Pointer to Frame status
	//  Return Value	: TRUE if successful
	BOOL(*set_frame_status)(void *editp, int n, FRAME_STATUS *fsp);

	//	Check if frame was saved
	//	editp 	: Editing Handle
	//  n		: Frame number
	//  Return Value	: TRUE if saved
	BOOL(*is_saveframe)(void *editp, int n);

	//	Check if it is a Keyframe
	//	editp 	: Editing Handle
	//  n		: Frame number
	//  Return Value	: TRUE for Keyframe
	BOOL(*is_keyframe)(void *editp, int n);

	//	Check if re-compression is required
	//	editp 	: Editing Handle
	//  n		: Frame number
	//  Return Value	: TRUE for required
	BOOL(*is_recompress)(void *editp, int n);

	//	ReDraw sliders and checkboxes of setting dialog
	//	fp	 	: pointer to FILTER Struct
	//  Return Value	: TRUE if successful
	BOOL(*filter_window_update)(void *fp);

	//	Check if setting dialog is being shown
	//	fp	 	: pointer to FILTER Struct
	//  Return Value	: TRUE if being displayed
	BOOL(*is_filter_window_disp)(void *fp);

	// Obtains info on the file being edited
	//	editp 	: Editing Handle
	//  fip		Pointer to FileInfo Struct
	//  Return Value	: TRUE if successful
	BOOL(*get_file_info)(void *editp, FILE_INFO *fip);

	// Obtains Current Profile name
	//	editp 	: Editing Handle
	//  n		: Profile Environment number
	//  Return Value	: pointer to Profile name (Return NULL when Failed)
	LPSTR(*get_config_name)(void *editp, int n);

	//	Check if filter is active
	//	fp	 	: pointer to FILTER Struct
	//  Return Value	: TRUE is active
	BOOL(*is_filter_active)(void *fp);

	//	Read Image Data of specified Frame as DIB(RGB24bit)
	//	This is the post-filtered image
	//	editp 	: Editing Handle
	//	n		: Frame number
	//  pixelp	: pointer to DIB-formatted data (return image size only when NULL)
	//	w,h		: Image size (Return DIB data only when NULL)
	//  Return Value	: TRUE if successful
	BOOL(*get_pixel_filtered)(void *editp, int n, void *pixelp, int *w, int *h);

	//	Read Audio Data of specified Frame
	//	Post-filtered data
	//	editp* 	: Editing Handle
	//	n		: Frame number
	//	buf 	: Storage buffer (Return sampling count only when NULL)
	//  Return Value	: Imported sampling count
	int(*get_audio_filtered)(void *editp, int n, void *buf);

	// Obtains Starting and Ending Frames of selection
	//	editp 	: Editing Handle
	//	s		: Starting frame of selection
	//	e		: Ending frame of selection
	//  Return Value	: TRUE if successful
	BOOL(*get_select_frame)(void *editp, int *s, int *e);

	//	Set Starting and Ending Frames of selection
	//	editp 	: Editing Handle
	//	s		: Starting frame of selection
	//	e		: Ending frame of selection
	//  Return Value	: TRUE if successful
	BOOL(*set_select_frame)(void *editp, int s, int e);

	// Convert from PIXEL to PIXEL_YC
	//	ycp		Pointer to PIXEL_YC Struct
	//	pixelp 	Pointer to PIXEL Struct
	//	w		:  No. of Struct
	//  Return Value	: TRUE if successful
	BOOL(*rgb2yc)(PIXEL_YC *ycp, PIXEL *pixelp, int w);

	// Convert from PIXEL_YC to PIXEL
	//	pixelp 	Pointer to PIXEL Struct
	//	ycp		Pointer to PIXEL_YC Struct
	//	w		:  No. of Struct
	//  Return Value	: TRUE if successful
	BOOL(*yc2rgb)(PIXEL *pixelp, PIXEL_YC *ycp, int w);

	// Obtains Import filename passed from File Dialog
	//	name	: Pointer to filename
	//	filter	: File filter
	//  def		: Default filename
	//  Return Value	: TRUE if successful, FALSE for Cancel
	BOOL(*dlg_get_load_name)(LPSTR name, LPSTR filter, LPSTR def);

	// Obtains save filename passed from File Dialog
	//	name	: Pointer to filename
	//	filter	: File filter
	//  def		: Default filename
	//  Return Value	: TRUE if successful, FALSE for Cancel
	BOOL(*dlg_get_save_name)(LPSTR name, LPSTR filter, LPSTR def);

	// Read value from INI file
	//	fp	 	: Pointer to FILTER Struct
	//	key		: Name for access key
	//  n		: Default value
	//  Return Value	: Read value
	int(*ini_load_int)(void *fp, LPSTR key, int n);

	//	Write value to INI file
	//	fp	 	: Pointer to FILTER Struct
	//	key		: Name for access key
	//  n		: Value to write
	//  Return Value	: Written value
	int(*ini_save_int)(void *fp, LPSTR key, int n);

	// Read text from INI file
	//	fp	 	: Pointer to FILTER Struct
	//	key		: Name for access key
	//  str		: Text buffer
	//  def		: Default text
	//  Return Value	: TRUE if successful
	BOOL(*ini_load_str)(void *fp, LPSTR key, LPSTR str, LPSTR def);

	//	Write text to INI file
	//	fp	 	: Pointer to FILTER Struct
	//	key		: Name for access key
	//  n		: Text to write
	//  Return Value	: TRUE if successful
	BOOL(*ini_save_str)(void *fp, LPSTR key, LPSTR str);

	// Obtains specified FileInfo from file ID
	//	editp 	: Editing Handle
	//  fip		Pointer to FileInfo Struct
	//	souce_file_id
	//			: File ID
	//  Return Value	: TRUE if successful
	BOOL(*get_source_file_info)(void *editp, FILE_INFO *fip, int source_file_id);

	// Obtains source file's ID and frame count for a given frame
	//	editp 	: Editing Handle
	//	n		: Frame number
	//	souce_file_id
	//			: pointer to file ID
	//	souce_video_number
	//			: pointer to frame count
	//  Return Value	: TRUE if successful
	BOOL(*get_source_video_number)(void *editp, int n, int *source_file_id, int *source_video_number);

	// Obtains System Info
	//	editp 	: Editing Handle (set NULL to invalidate all flags and filename associated with sip)
	//  sip		Pointer to SystemInfo Struct
	//  Return Value	: TRUE if successful
	BOOL(*get_sys_info)(void *editp, SYS_INFO *sip);

	// Obtains the pointer to FILTER Struct with a specific Filter ID
	//	filter_id
	//		 	: Filter ID (0Å`Total filter count-1)
	//  Return Value	: pointer to Filter Struct (Return NULL when Failed)
	void 		*(*get_filterp)(int filter_id);

	//	Å¶Please use get_ycp_filtering_cache_ex() instead whenever possible
	// Obtains Image Data's pointer of specified Frame
	//	The image is the one JUST before processing by current filter
	//	fp	 	: Pointer to FILTER Struct
	//	editp 	: Editing Handle
	//	n	 	: Frame number
	//	reserve	: NULL
	//  Return Value	: pointer to Image Data (Return NULL when Failed)
	//			  Content of Image Data pointer is valid until external function or Main returns
	void		*(*get_ycp_filtering)(void *fp, void *editp, int n, void *reserve);

	//	Read Audio Data from specified Frame
	//	The data is the one JUST before processing by current filter
	//	fp	 	: Pointer to FILTER Struct
	//	editp 	: Editing Handle
	//	n		: Frame number
	//	buf 	: Storage buffer (NULL for getting sample count only)
	//  Return Value	: Imported sample count
	int(*get_audio_filtering)(void *fp, void *editp, int n, void *buf);

	//	Set cache for get_ycp_filtering_cache_ex()
	//	Cache is re-allocated when setting changed
	//	Cache is allocated only when filter is active
	//	fp	 	: Pointer to FILTER Struct
	//	w	 	: Width of cache area
	//	h	 	: Height of cache area
	//	d	 	: Number of frames to cache
	//	flag 	: NULL
	//  Return Value	: TRUE if successful
	BOOL(*set_ycp_filtering_cache_size)(void *fp, int w, int h, int d, int flag);

	//	Å¶Please use get_ycp_filtering_cache_ex() instead whenever possible
	// Obtains Image Data's cache pointer for specified Frame
	//	Follow cache settings defined by set_ycp_filtering_cache_size()
	//	The data is the one just before processing by current filter
	//	fp	 	: Pointer to FILTER Struct
	//	editp 	: Editing Handle
	//	n	 	: Frame number
	//  Return Value	: cache pointer to Image Data (Return NULL when Failed)
	// Content in pointer will be valid until cache is discarded
	void		*(*get_ycp_filtering_cache)(void *fp, void *editp, int n);

	// Obtains Image Data's pointer of specified Frame
	//	The data are those before any filtering
	//	editp 	: Editing Handle
	//	n	 	: Frame number
	//	ofs	 	: Frame offset w.r.t. original AVI
	//  Return Value	: pointer to Image Data (Return NULL when Failed)
	// Content in pointer will be valid until cache is discarded
	void		*(*get_ycp_source_cache)(void *editp, int n, int ofs);

	// Obtains Image Data's pointer for the frame being displayed
	//	Post-filtered data
	//	Usable in Display/Window Filter only
	//	editp 	: Editing Handle
	//	format	: Image format( NULL = RGB24bit / 'Y''U''Y''2' = YUY2 )
	//  Return Value	: pointer to Image Data (Return NULL when Failed)
	//			  Content of Image Data pointer is valid until external function or Main returns
	void		*(*get_disp_pixelp)(void *editp, DWORD format);

	//	Read Image Data from specified Frame
	//	The data are those before any filtering
	//	editp 	: Editing Handle
	//	n	 	: Frame number
	//  pixelp	: pointer to DIB-format data
	//	format	: Image format( NULL = RGB24bit / 'Y''U''Y''2' = YUY2 )
	//  Return Value	: TRUE if successful
	BOOL(*get_pixel_source)(void *editp, int n, void *pixelp, DWORD format);

	//	Read Image Data from specified Frame
	//	Post-filtered data
	//	editp 	: Editing Handle
	//	n	 	: Frame number
	//  pixelp	: pointer to DIB-format data pointer (set NULL to return image size only)
	//	w,h		: Image size (set NULL to return DIB-format data only)
	//	format	: Image format( NULL = RGB24bit / 'Y''U''Y''2' = YUY2 )
	//  Return Value	: TRUE if successful
	BOOL(*get_pixel_filtered_ex)(void *editp, int n, void *pixelp, int *w, int *h, DWORD format);

	// Obtains Image cache pointer of specified Frame
	//	Follow cache setting defined by set_ycp_filtering_cache_size()
	//	The data is the one just before processing by current filter
	//	fp	 	: Pointer to FILTER Struct
	//	editp 	: Editing Handle
	//	n	 	: Frame number
	//	w,h		: Obtained Image size (set NULL to ignore)
	//  Return Value	: Cache pointer to Image Data (Return NULL when Failed)
	// Content referred by pointer is valid unitl cache is discarded
	PIXEL_YC	*(*get_ycp_filtering_cache_ex)(void *fp, void *editp, int n, int *w, int *h);

	//	Call specified function with threads as defined in system settings
	// Please DO NOT call Win32API or External functions( except rgb2yc,yc2rgb) inside this function
	//	func	: The function to be called
	//	param1 	: General parameter passable to the called function
	//	param2 	: General parameter passable to the called function
	//  Return Value	: TRUE if successful
	BOOL(*exec_multi_thread_func)(MULTI_THREAD_FUNC func, void *param1, void *param2);

	//	Make empty image data
	//	Though this can be used by external function just like ycp_edit,
	//	this can not be swapped with ycp_edit,ycp_temp in FILTER_PROC_INFO
	//  Return Value	: pointer to created Image Data (Return NULL when Failed)
	PIXEL_YC	*(*create_yc)(void);

	//	Delete object created by create_yc
	void(*delete_yc)(PIXEL_YC *ycp);

	// Read image data from BMP file
	//	ycp     : pointer to the frame where the image is to be imported (set NULL to skip updating image)
	//	file	: BMP filename
	//	w,h		: Imported Image size (can be set to NULL)
	//	flag 	: NULL
	//  Return Value	: TRUE if successful
	BOOL(*load_image)(PIXEL_YC *ycp, LPSTR file, int *w, int *h, int flag);

	//	Resize image data
	//	Transform original image to any arbitary size
	//	ycp     : pointer to resized image data
	//	w,h     : target resolution
	//	ycp_src	: Pointer to original image data( same as ycp if set to NULL)
	//	sx,sy	: Coordinate of Top-left corner of original image w.r.t. resized image
	//	sw,sh	: Size of original image w.r.t. resized image
	void(*resize_yc)(PIXEL_YC *ycp, int w, int h, PIXEL_YC *ycp_src, int sx, int sy, int sw, int sh);

	//	Copy arbitary area from an image data
	//	The image will be clipped to fit max resolution
	//	Please do not overlap the area of source and destination
	//	ycp     Pointer to copy destination
	//	x,y		: Top-left corner of destination
	//	ycp_src	Pointer to source image
	//	sx,sy	: Top-left coordinate of source
	//	sw,sh	: size of source
	//	tr      : Transparency of source (0Å`4096)
	void(*copy_yc)(PIXEL_YC *ycp, int x, int y, PIXEL_YC *ycp_src, int sx, int sy, int sw, int sh, int tr);

	//	Draw text on frame image
	//	Clipping may occur to fit max resolution
	//	ycp     : pointer to Image Data (NULL to skip drawing and just return the size)
	//	x,y		: Top-left of the drawing coordinate
	//	text	: The text itself
	//	r,g,b	: Color (0Å`255)
	//	tr      : Transparency (0Å`4096)
	//	hfont	: Font (NULL for default font)
	//	w,h		: Size of the resulting drawn text (can be NULL)
	void(*draw_text)(PIXEL_YC *ycp, int x, int y, LPSTR text, int r, int g, int b, int tr, HFONT hfont, int *w, int *h);

	//	Obtains a handle for use with avi_file_read_video() and avi_file_read_audio()
	//  when opening AVI file
	//	Å¶File and format can be different from the one being edited
	//	file    : AVI filename (also accepts file that passthrough import plugins)
	//  fip		: pointer to FileInfo Struct (Contains info for imported file)
	//	flag 	: Open flags
	//	AVI_FILE_OPEN_FLAG_ONLY_YUY2		: Expand to YUY2
	//	AVI_FILE_OPEN_FLAG_ONLY_RGB24		: Expand to RGB24
	//	AVI_FILE_OPEN_FLAG_ONLY_RGB32		: Expand to RGB32
	//	AVI_FILE_OPEN_FLAG_VIDEO_ONLY		: Read video stream only
	//	AVI_FILE_OPEN_FLAG_AUDIO_ONLY		: Read audio stream only
	//  Return Value	: AVI file handle (Return NULL when Failed)
	AVI_FILE_HANDLE(*avi_file_open)(LPSTR file, FILE_INFO *fip, int flag);

	//	Close AVI file
	//	afh		: AVI file handle
	void(*avi_file_close)(AVI_FILE_HANDLE afh);

	// Read Image Data from AVI file
	//	afh		: AVI file handle
	//	ycp     Pointer to Image Data
	//	n		: Frame number
	//  Return Value	: TRUE if successful
	BOOL(*avi_file_read_video)(AVI_FILE_HANDLE afh, PIXEL_YC *ycp, int n);

	// Read audio from AVI file
	//	afh		: AVI file handle
	//	buf     Pointer to audio buffer
	//	n		: Frame number
	//  Return Value	: Opened sample count
	int(*avi_file_read_audio)(AVI_FILE_HANDLE afh, void *buf, int n);

	// Obtains DIB Image Data's pointer from AVI file
	//	Image format depends on video decode format as defined in 
	//	FILE_INFO, which in turn was obtained from avi_file_open()
	//	afh		: AVI file handle
	//	n		: Frame number
	//  Return Value	: pointer to DIB-format data (Return NULL when Failed)
	//			  Content of Image Data pointer is valid until external function or Main returns
	void 		*(*avi_file_get_video_pixelp)(AVI_FILE_HANDLE afh, int n);

	// Obtains the file filter(format) as supported by avi_file_open()
	//	type	: file type
	//	GET_AVI_FILE_FILTER_TYPE_VIDEO	: Video
	//	GET_AVI_FILE_FILTER_TYPE_AUDIO	: Audio
	//  Return Value	Pointer to File filter
	LPSTR(*get_avi_file_filter)(int type);

	// Read audio data from AVI file
	//	afh		: AVI file handle
	//	start   : sample number for reading to begin with
	//	length	: sample counts to be read
	//	buf		Pointer to audio buffer
	//  Return Value	: Imported sample count
	int(*avi_file_read_audio_sample)(AVI_FILE_HANDLE afh, int start, int length, void *buf);

	// Change sampling rate an others when using avi_file_read_audio_sample()
	//	afh		: AVI file handle
	//	audio_rate	: Sampling rate
	//	audio_ch	: Channel count
	//  Return Value	: Total sample count w.r.t. changed sampling rate
	int(*avi_file_set_audio_sample_rate)(AVI_FILE_HANDLE afh, int audio_rate, int audio_ch);

	// Obtains pointer to Frame status buffer
	//	editp 	: Editing Handle
	//  type	: type of status
	//	FARME_STATUS_TYPE_EDIT_FLAG	: Editing flag
	//	FARME_STATUS_TYPE_INTER		: Interlace flag
	//  Return Value	Pointer to buffer
	//			  Content referenced by pointer is valid until file is closed
	BYTE		*(*get_frame_status_table)(void *editp, int type);

	//	Set current editing status in Undo buffer
	//	editp 	: Editing Handle
	//  Return Value	: TRUE if successful
	BOOL(*set_undo)(void *editp);

	//	Add a new item under the Setting/Filter+ menu
	//	When the menu is selected, WM_FILTER_COMMAND's message will pass to
	//	specific window via hwnd
	// Please call this function at least once from  either func_init() or WM_FILTER_INIT
	//	fp	 	: Pointer to FILTER Struct
	//	name 	: menu name
	//	hwnd 	: Window Handle that sends WM_FILTER_COMMAND
	//	id	 	: WPARAM of WM_FILTER_COMMAND
	//	def_key	: Virtual keycode for Shortcut key (NULL for no shortcut)
	//	flag	: flags...
	//	ADD_MENU_ITEM_FLAG_KEY_SHIFT	: Use SHIFT for shortcut key
	//	ADD_MENU_ITEM_FLAG_KEY_CTRL		: Use Ctrl for shortcut key
	//	ADD_MENU_ITEM_FLAG_KEY_ALT		: Use Alt for shortcut key
	//  Return Value	: TRUE if successful
	BOOL(*add_menu_item)(void *fp, LPSTR name, HWND hwnd, int id, int def_key, int flag);

	//	Open file for editing
	//	editp 	: Editing Handle
	//	file 	: filename
	//	flag 	: flags...
	//	EDIT_OPEN_FLAG_ADD			: Add/Append to current session
	//	EDIT_OPEN_FLAG_AUDIO		: Import audio
	//	EDIT_OPEN_FLAG_PROJECT		: Open project file
	//	EDIT_OPEN_FLAG_DIALOG		: Show Open File dialog box
	//  Return Value	: TRUE if successful
	BOOL(*edit_open)(void *editp, LPSTR file, int flag);

	//	Close editing file
	//	editp 	: Editing Handle
	//  Return Value	: TRUE if successful
	BOOL(*edit_close)(void *editp);

	//	Export editing data to AVI
	//	Also allows "Export to WAV" or "Export with Plugin"
	//	editp 	: Editing Handle
	//	file 	: Output filename
	//	flag	: flags...
	//	EDIT_OUTPUT_FLAG_NO_DIALOG	: No export dialog
	//	EDIT_OUTPUT_FLAG_WAV		: Export WAV
	//	type	: name of export plugin (NULL for AVI/WAV Export)
	//  Return Value	: TRUE if successful
	BOOL(*edit_output)(void *editp, LPSTR file, int flag, LPSTR type);

	//	Set profile
	//	editp 	: Editing Handle
	//  n		: profile environment's number
	//  name	: Profile name
	//  Return Value	: TRUE if successful
	BOOL(*set_config)(void *editp, int n, LPSTR name);
	int			reserve[7];
} EXFUNC;
#define	AVI_FILE_OPEN_FLAG_VIDEO_ONLY		16
#define	AVI_FILE_OPEN_FLAG_AUDIO_ONLY		32
#define	AVI_FILE_OPEN_FLAG_ONLY_YUY2		0x10000
#define	AVI_FILE_OPEN_FLAG_ONLY_RGB24		0x20000
#define	AVI_FILE_OPEN_FLAG_ONLY_RGB32		0x40000
#define	GET_AVI_FILE_FILTER_TYPE_VIDEO		0
#define	GET_AVI_FILE_FILTER_TYPE_AUDIO		1
#define FARME_STATUS_TYPE_EDIT_FLAG			0
#define FARME_STATUS_TYPE_INTER				1
#define ADD_MENU_ITEM_FLAG_KEY_SHIFT		1
#define ADD_MENU_ITEM_FLAG_KEY_CTRL			2
#define ADD_MENU_ITEM_FLAG_KEY_ALT			4
#define	EDIT_OPEN_FLAG_ADD					2
#define	EDIT_OPEN_FLAG_AUDIO				16
#define	EDIT_OPEN_FLAG_PROJECT				512
#define	EDIT_OPEN_FLAG_DIALOG				65536
#define	EDIT_OUTPUT_FLAG_NO_DIALOG			2
#define	EDIT_OUTPUT_FLAG_WAV				4

//	ÉtÉBÉãÉ^ Struct
typedef struct {
	int		flag;				//	Filter Flags...
	//	FILTER_FLAG_ALWAYS_ACTIVE		: Filter is always active
	//	FILTER_FLAG_CONFIG_POPUP		: Use pop-up menu for setting dialog
	//	FILTER_FLAG_CONFIG_CHECK		: checkboxes became menu items under Settings/Filter+
	//	FILTER_FLAG_CONFIG_RADIO		: Only one of the checkboxs can be enabled at one time(i.e. mutually exclusive options)
	//	FILTER_FLAG_EX_DATA				: Extended data can be saved
	//	FILTER_FLAG_PRIORITY_HIGHEST	: Filter always has highest priority
	//	FILTER_FLAG_PRIORITY_LOWEST		: Filter always has lowest priority
	//	FILTER_FLAG_WINDOW_THICKFRAME	: Window size can be changed by user
	//	FILTER_FLAG_WINDOW_SIZE			: Window size can be defined
	//	FILTER_FLAG_DISP_FILTER			: This is a Display/Window filter
	//	FILTER_FLAG_REDRAW				: Let plugin to handle refresh
	//	FILTER_FLAG_EX_INFORMATION		: Allows setting of extra info for filter
	//	FILTER_FLAG_INFORMATION			: Please use FILTER_FLAG_EX_INFORMATION instead
	//	FILTER_FLAG_NO_CONFIG			: Hide setting window
	//	FILTER_FLAG_AUDIO_FILTER		: Make this an audio filter
	//	FILTER_FLAG_RADIO_BUTTON		: Change checkbox to radio button
	//	FILTER_FLAG_WINDOW_HSCROLL		: Add a horizontal scroll bar
	//	FILTER_FLAG_WINDOW_VSCROLL		: Add a vertical scroll bar
	//	FILTER_FLAG_INTERLACE_FILTER	: Make this a deinterlace filter
	//	FILTER_FLAG_NO_INIT_DATA		: Do not initialize image data for func_proc()
	//	FILTER_FLAG_IMPORT				: Add item to Import menu
	//	FILTER_FLAG_EXPORT				: Add item to Export menu
	//	FILTER_FLAG_MAIN_MESSAGE		: func_WndProc()can send out WM_FILTER_MAIN_??? messages
	int		x, y;				//	When defining window size (FILTER_FLAG_WINDOW_SIZE)
	//	Use FILTER_WINDOW_SIZE_CLIENT to define size by Client area.
	//  Use FILTER_WINDOW_SIZE_ADD to define Extra area on top of standard size.
	TCHAR	*name;				//	Filter name
	int		track_n;			//	Slider count
	TCHAR	**track_name;		//	pointer to sliders' names (can be NULL if no slider used)
	int		*track_default;		//	pointer to sliders' default values (can be NULL if no slider used)
	int		*track_s, *track_e;	//	Sliders' range (NULL for all 0-256)
	int		check_n;			//	number of checkbox
	TCHAR	**check_name;		//	pointer to checkboxes' names(can be NULL if no checkbox)
	int		*check_default;		//	pointer to checkboxes's default values (can be NULL if no checkbox)
	//	The checkbox will becomes a BUTTON when default value is NEGATIVE. When the button is pushed, WM_COMMAND( WPARAM = MID_FILTER_BUTTON + n ) will be sent out.
	BOOL(*func_proc)(void *fp, FILTER_PROC_INFO *fpip);
	//	pointer to filtering function (skip calling if NULL)
	BOOL(*func_init)(void *fp);
	//	pointer to initialization function (skip calling if NULL)
	BOOL(*func_exit)(void *fp);
	//	pointer to exit function (skip calling if NULL)
	BOOL(*func_update)(void *fp, int status);
	//	pointer to function to be called whenever settings is changed (skip calling if NULL)
	//	FILTER_UPDATE_STATUS_ALL		: All parameters was changed
	//	FILTER_UPDATE_STATUS_TRACK + n	: The n-th slider was changed
	//	FILTER_UPDATE_STATUS_CHECK + n	: The n-th checkbox was changed
	BOOL(*func_WndProc)(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, void *editp, void *fp);
	//	pointer to function to be called whenever window message arrives(skip calling if NULL)
	//	Not to be called when VFAPI is working
	//	The following extra messages are sent on top of the standard one
	//	WM_FILTER_UPDATE		: Sent immediately after filter setting is changed or editing content modified
	//	WM_FILTER_FILE_OPEN		: Sent after file is opened for editing
	//	WM_FILTER_FILE_UPDATE	: After editing file is updated(e.g. Appending file, Importing audio)
	//	WM_FILTER_FILE_CLOSE	: Sent JUST before editing file is closed
	//	WM_FILTER_INIT			: After initialization
	//	WM_FILTER_EXIT			: Just before exit
	//	WM_FILTER_SAVE_START	: Just before save
	//	WM_FILTER_SAVE_END		: Just after save
	//	WM_FILTER_IMPORT		: Just after selecting import
	//	WM_FILTER_EXPORT		: Just after selecting export
	//	WM_FILTER_CHANGE_ACTIVE	: Just after filter toggle
	//	WM_FILTER_CHANGE_WINDOW	: After the filter window's hidden state was changed
	//	WM_FILTER_CHANGE_PARAM	: Just after parameters for this filter was changed
	//	WM_FILTER_CHANGE_EDIT	: Just after switching editng/non-editing mode
	//	The following messages will be sent ONLY WHEN FILTER_FLAG_MAIN_MESSAGE is enabled
	//	WM_FILTER_MAIN_MOUSE_DOWN	: Left mouse press on main window
	//	WM_FILTER_MAIN_MOUSE_UP		: Left mouse release on main window
	//	WM_FILTER_MAIN_MOUSE_MOVE	: Mouse move on main window
	//	WM_FILTER_MAIN_MOUSE_DBLCLK	: Double click on main window
	//	WM_FILTER_MAIN_MOUSE_R_DOWN	: Right mouse down 
	//	WM_FILTER_MAIN_MOUSE_R_UP	: Right mouse release
	//	WM_FILTER_MAIN_MOUSE_WHEEL	: Mouse scroll
	//	WM_FILTER_MAIN_KEY_DOWN		: Some key is pressed
	//	WM_FILTER_MAIN_KEY_UP		: Some key is released
	//	WM_FILTER_MAIN_MOVESIZE		: When main window size or position has changed
	//	WM_FILTER_MAIN_CONTEXTMENU	: When context menu is invoked on main window
	//	WM_FILTER_MAIN_MOUSE_???'s lparam holds the mouse coordinate w.r.t. editing image (0 when not editing)
	//	WM_FILTER_MAIN_MOUSE_WHEEL's wparam holds wheel's turn amount in its upper WORD
	//	WM_FILTER_MAIN_KEY_???'s wparam holds virtual key code
	//	WM_FILTER_MAIN_MOVESIZE's lparam holds  main windows's Window Handle
	//	WM_FILTER_MAIN_CONTEXTMENU's lparam holds screen coordinate
	//	When menu is shown using WM_FILTER_MAIN_CONTEXTMENU please set Return Value to FALSE(no refresh)
	//	When Return Value is TRUE, editing content is completely redrawn
	int		*track;				//	pointer to sliders' settings (Set by AviUtl)
	int		*check;				//	pointer to checkboxes' setting  (Set by AviUtl)
	void	*ex_data_ptr;		//	pointer to extended data (when FILTER_FLAG_EX_DATA is set)
	int		ex_data_size;		//	size of extended data ( when FILTER_FLAG_EX_DATAÇ™ is set)
	TCHAR	*information;		//	pointer to filter info (when FILTER_FLAG_EX_INFORMATION is set)
	BOOL(*func_save_start)(void *fp, int s, int e, void *editp);
	//	pointer to function to be called just before saving begins (skip calling if NULL)
	//	s	 	: First frame to be saved
	//	e 		: Last frame to be saved
	//  Return Value	: TRUE if successful
	BOOL(*func_save_end)(void *fp, void *editp);
	//	pointer to function to be called just before saving ends (skip calling if NULL)
	EXFUNC	*exfunc;			//	pointer to external function table (set by AviUtl)
	HWND	hwnd;				//	Window Handle (Set by AviUtl)
	HINSTANCE	dll_hinst;		//	Instance handle for DLL (Set by AviUtl)
	void	*ex_data_def;		//	pointer to initial data area of extended data (no initialization if NULL)
	BOOL(*func_is_saveframe)(void *fp, void *editp, int saveno, int frame, int fps, int edit_flag, int inter);
	//	pointer to function to be called when a deinterlace filter decided which frame(s) should be saved (skip calling if NULL)
	//  saveno		: First frame that should be saved
	//	frame		: Editing frame number
	//	fps			: settings for changing framerate (30,24,20,15,10)
	//	edit_flag	: Edit flags
	//	inter		: interlace state of frame
	//	Return Value		: TRUE for Frame to be savedÅAFALSE for Frame to be dropped
	BOOL(*func_project_load)(void *fp, void *editp, void *data, int size);
	// pointer to function to be called when opening project file(skip calling if NULL)
	//	Will not be called when project is empty
	// data 	: pointer to project data
	// size 	: imported data size in bytes
	//  Return Value	: TRUE if successful
	BOOL(*func_project_save)(void *fp, void *editp, void *data, int *size);
	//	pointer to function to be called when saving project file (skip calling if NULL)
	//	Filter data is saved into project file
	// In order to get saving size, AviUtl will initially call this function with data=NULL, then it will call again to get actual data.
	//	data 	: pointer to Storage buffer for project data to be written(NULL to return only the data size in bytes)
	//	size 	: pointer to data size to be written
	//  Return Value	: TRUE if there are data to be saved
	BOOL(*func_modify_title)(void *fp, void *editp, int frame, LPSTR title, int max_title);
	//	pointer to function to be called when title bar of main window is displayed (skip calling if NULL)
	//	Allows changing the text of title bar (will not be invoked when not editing or exporting file)
	//	frame		: Editing frame number
	//	title 		: Text to be displayed
	//	max_title 	: buffer size of title
	//  Return Value	: TRUE if successful
	TCHAR	*dll_path;			//	When a sub-directory of .\Plugins contains DLL, such folder name will be stored here.
	int		reserve[2];			//	Reserved for expansion. Set to NULL.

} FILTER;
#define	FILTER_FLAG_ACTIVE				1
#define	FILTER_FLAG_ALWAYS_ACTIVE		4
#define	FILTER_FLAG_CONFIG_POPUP		8
#define	FILTER_FLAG_CONFIG_CHECK		16
#define	FILTER_FLAG_CONFIG_RADIO		32
#define	FILTER_FLAG_EX_DATA				1024
#define	FILTER_FLAG_PRIORITY_HIGHEST	2048
#define	FILTER_FLAG_PRIORITY_LOWEST		4096
#define	FILTER_FLAG_WINDOW_THICKFRAME	8192
#define	FILTER_FLAG_WINDOW_SIZE			16384
#define	FILTER_FLAG_DISP_FILTER			32768
#define	FILTER_FLAG_REDRAW				0x20000
#define	FILTER_FLAG_EX_INFORMATION		0x40000
#define	FILTER_FLAG_INFORMATION			0x80000
#define	FILTER_FLAG_NO_CONFIG			0x100000
#define	FILTER_FLAG_AUDIO_FILTER		0x200000
#define	FILTER_FLAG_RADIO_BUTTON		0x400000
#define	FILTER_FLAG_WINDOW_HSCROLL		0x800000
#define	FILTER_FLAG_WINDOW_VSCROLL		0x1000000
#define	FILTER_FLAG_INTERLACE_FILTER	0x4000000
#define	FILTER_FLAG_NO_INIT_DATA		0x8000000
#define	FILTER_FLAG_IMPORT				0x10000000
#define	FILTER_FLAG_EXPORT				0x20000000
#define	FILTER_FLAG_MAIN_MESSAGE		0x40000000
#define WM_FILTER_UPDATE				(WM_USER+100)
#define WM_FILTER_FILE_OPEN				(WM_USER+101)
#define WM_FILTER_FILE_CLOSE			(WM_USER+102)
#define WM_FILTER_INIT					(WM_USER+103)
#define WM_FILTER_EXIT					(WM_USER+104)
#define WM_FILTER_SAVE_START			(WM_USER+105)
#define WM_FILTER_SAVE_END				(WM_USER+106)
#define WM_FILTER_IMPORT				(WM_USER+107)
#define WM_FILTER_EXPORT				(WM_USER+108)
#define WM_FILTER_CHANGE_ACTIVE			(WM_USER+109)
#define WM_FILTER_CHANGE_WINDOW			(WM_USER+110)
#define WM_FILTER_CHANGE_PARAM			(WM_USER+111)
#define WM_FILTER_CHANGE_EDIT			(WM_USER+112)
#define WM_FILTER_COMMAND				(WM_USER+113)
#define	WM_FILTER_FILE_UPDATE			(WM_USER+114)
#define	WM_FILTER_MAIN_MOUSE_DOWN		(WM_USER+120)
#define	WM_FILTER_MAIN_MOUSE_UP			(WM_USER+121)
#define	WM_FILTER_MAIN_MOUSE_MOVE		(WM_USER+122)
#define	WM_FILTER_MAIN_KEY_DOWN			(WM_USER+123)
#define	WM_FILTER_MAIN_KEY_UP			(WM_USER+124)
#define	WM_FILTER_MAIN_MOVESIZE			(WM_USER+125)
#define	WM_FILTER_MAIN_MOUSE_DBLCLK		(WM_USER+126)
#define	WM_FILTER_MAIN_MOUSE_R_DOWN		(WM_USER+127)
#define	WM_FILTER_MAIN_MOUSE_R_UP		(WM_USER+128)
#define	WM_FILTER_MAIN_MOUSE_WHEEL		(WM_USER+129)
#define	WM_FILTER_MAIN_CONTEXTMENU		(WM_USER+130)
#define FILTER_UPDATE_STATUS_ALL		0
#define FILTER_UPDATE_STATUS_TRACK		0x10000
#define FILTER_UPDATE_STATUS_CHECK		0x20000
#define	FILTER_WINDOW_SIZE_CLIENT		0x10000000
#define	FILTER_WINDOW_SIZE_ADD			0x30000000

//	Struct for Filter DLL
typedef struct {
	int			flag;
	int			x, y;
	TCHAR		*name;
	int			track_n;
	TCHAR		**track_name;
	int			*track_default;
	int			*track_s, *track_e;
	int			check_n;
	TCHAR		**check_name;
	int			*check_default;
	BOOL(*func_proc)(FILTER *fp, FILTER_PROC_INFO *fpip);
	BOOL(*func_init)(FILTER *fp);
	BOOL(*func_exit)(FILTER *fp);
	BOOL(*func_update)(FILTER *fp, int status);
	BOOL(*func_WndProc)(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, void *editp, FILTER *fp);
	int			*track, *check;
	void		*ex_data_ptr;
	int			ex_data_size;
	TCHAR		*information;
	BOOL(*func_save_start)(FILTER *fp, int s, int e, void *editp);
	BOOL(*func_save_end)(FILTER *fp, void *editp);
	EXFUNC		*exfunc;
	HWND		hwnd;
	HINSTANCE	dll_hinst;
	void		*ex_data_def;
	BOOL(*func_is_saveframe)(FILTER *fp, void *editp, int saveno, int frame, int fps, int edit_flag, int inter);
	BOOL(*func_project_load)(FILTER *fp, void *editp, void *data, int size);
	BOOL(*func_project_save)(FILTER *fp, void *editp, void *data, int *size);
	BOOL(*func_modify_title)(FILTER *fp, void *editp, int frame, LPSTR title, int max_title);
	TCHAR		*dll_path;
	int			reserve[2];
} FILTER_DLL;

#define	MID_FILTER_BUTTON			12004

BOOL func_proc(FILTER *fp, FILTER_PROC_INFO *fpip);
BOOL func_init(FILTER *fp);
BOOL func_exit(FILTER *fp);
BOOL func_update(FILTER *fp, int status);
BOOL func_WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, void *editp, FILTER *fp);
BOOL func_save_start(FILTER *fp, int s, int e, void *editp);
BOOL func_save_end(FILTER *fp, void *editp);
BOOL func_is_saveframe(FILTER *fp, void *editp, int saveno, int frame, int fps, int edit_flag, int inter);
BOOL func_project_load(FILTER *fp, void *editp, void *data, int size);
BOOL func_project_save(FILTER *fp, void *editp, void *data, int *size);
BOOL func_modify_title(FILTER *fp, void *editp, int frame, LPSTR title, int max_title);


