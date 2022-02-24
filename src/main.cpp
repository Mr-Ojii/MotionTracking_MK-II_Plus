#include <Windows.h>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
//#include <stdexcept>
#include "filter.h"
#include "opencv2\core\utility.hpp"
#include "opencv2\highgui.hpp"
#include "opencv2\tracking\tracker.hpp"
#include "opencv2\objdetect.hpp"
#include "opencv2\video.hpp"
#include "resource.h"

#define	TRACK_N	1														//	トラックバーの数
TCHAR	*track_name[] = { "Method" };	//	トラックバーの名前
int		track_default[] = { 2 };	//	トラックバーの初期値
int		track_s[] = { 1 };	//	トラックバーの下限値
int		track_e[] = { 7 };	//	トラックバーの上限値

#define	CHECK_N	10														//	チェックボックスの数
TCHAR	*check_name[] = { "1. Select Object", "2. Analyze", "3A. View Result", "3B. Clear Result", "4. As English EXO?", "5. As Sub-filter/部分フィルター?", "6. Save EXO", "Quick Blur", "Easy Privacy", "⑨HELP" };				//	チェックボックスの名前
int		check_default[] = { -1, -1, -1, -1, 0, 0, -1, 0, 0, -1 };				//	チェックボックスの初期値 (値は0か1)

#define METHOD_N 7
TCHAR* track_method[] = { "BOOSTING", "MIL", "MEDIANFLOW", "TLD", "KCF", "CSRT", "MOSSE" };

#define HSV_CHECK_N 4
TCHAR   *hsv_check_name[] = { "HSV", "Hue only", "Saturation only", "Value only" };
int     hsv_check_default[] = { 1, 0, 0, 0 };

#define BGS_CHECK_N 5
TCHAR *bgs_check_name[] = { "MOG2", "KNN","MOG2(Mask)", "KNN(Mask)", "HELP" };
int bgs_check_default[] = { 0, 0, 0, 1, -1 };

#define BGS_TRACK_N 5
TCHAR *bgs_track_name[] = { "Range", "Shadow","NMix", "BG%", "d2T" };
int bgs_track_default[] = { 30, 0, 5, 70, 400 };
int bgs_track_s[] = { 1, 0, 1, 1, 100 };
int bgs_track_e[] = { 250, 1, 30, 99, 1000 };

#ifdef __AVX__
TCHAR* verstr={ "MotionTracking MK-II Plus AVX by Mr-Ojii\0" };
#else
TCHAR* verstr = { "MotionTracking MK-II Plus SSE2 by Mr-Ojii\0" };
#endif
const TCHAR *help_text =
{
	"=Method=\n"
	"1. AdaBoost\n"
	"2. Multi Instance Learning\n"
	"3. MediaFlow\n"
	"4. TLD\n"
	"5. KCF\n"
	"6. CSRT\n"
	"7. MOSSE\n"
	"\n=Steps=\n"
	"0. Mark a section to track\n"
	"1. Click 1st button, Drag a box on the object to be tracked(in popup Window).\n"
	"   Close the popup Window.\n"
	"2. Click Analyze, wait for completion.\n"
	"3. Click View Result and check in the popup Window.\n"
	"IF result is good, click SaveEXO or check QuickBlur.\n"
	"Otherwise, click Clear Result and go back to step 0 or 1.\n\n"
	"=Save EXO=\n"
	"Auto correct for single sandwiched error result.\n"
	"Support CJK filename\n\n"
	"=Options=\n"
	"Quick Blur: Direct blur on AviUtl Window according to\n"
	"  tracking result.\n"
	"Easy Privacy: Blur all detected faces(real face only),\n"
	"  No tracking is needed.\n"
	"  Works well on frontal face, poor on profile face.\n\n"
	"⑨HELP: by チルノ\n"
	"\nLicense: 3-clause BSD License"
};

const TCHAR* bg_help = {
	"Common Parameters\n"
	"==================\n"
	"Range: Use <Range> no. of frames before and after\n"
	"           current frame for analysis.[30]\n"
	"Shadow: 1= Extract shadow [0]\n\n"
	"MOG2-Only\n"
	"===========\n"
	"NMix: Number of Gaussian mixtures [5]\n"
	"BG%: Background ratio [70%]\n\n"
	"kNN-Only\n"
	"==========\n"
	"d2T: Threshold on the squared distance between\n"
	"        the pixel and the sample to decide whether\n"
	"        a pixel is close to that sample.\n"
};

FILTER_DLL filter = {
	FILTER_FLAG_EX_INFORMATION,	//	フィルタのフラグ
	//	FILTER_FLAG_ALWAYS_ACTIVE		: フィルタを常にアクティブにします
	//	FILTER_FLAG_CONFIG_POPUP		: 設定をポップアップメニューにします
	//	FILTER_FLAG_CONFIG_CHECK		: 設定をチェックボックスメニューにします
	//	FILTER_FLAG_CONFIG_RADIO		: 設定をラジオボタンメニューにします
	//	FILTER_FLAG_EX_DATA				: 拡張データを保存出来るようにします。
	//	FILTER_FLAG_PRIORITY_HIGHEST	: フィルタのプライオリティを常に最上位にします
	//	FILTER_FLAG_PRIORITY_LOWEST		: フィルタのプライオリティを常に最下位にします
	//	FILTER_FLAG_WINDOW_THICKFRAME	: サイズ変更可能なウィンドウを作ります
	//	FILTER_FLAG_WINDOW_SIZE			: 設定ウィンドウのサイズを指定出来るようにします
	//	FILTER_FLAG_DISP_FILTER			: 表示フィルタにします
	//	FILTER_FLAG_EX_INFORMATION		: フィルタの拡張情報を設定できるようにします
	//	FILTER_FLAG_NO_CONFIG			: 設定ウィンドウを表示しないようにします
	//	FILTER_FLAG_AUDIO_FILTER		: オーディオフィルタにします
	//	FILTER_FLAG_RADIO_BUTTON		: チェックボックスをラジオボタンにします
	//	FILTER_FLAG_WINDOW_HSCROLL		: 水平スクロールバーを持つウィンドウを作ります
	//	FILTER_FLAG_WINDOW_VSCROLL		: 垂直スクロールバーを持つウィンドウを作ります
	//	FILTER_FLAG_IMPORT				: インポートメニューを作ります
	//	FILTER_FLAG_EXPORT				: エクスポートメニューを作ります
	0, 0,						//	設定ウインドウのサイズ (FILTER_FLAG_WINDOW_SIZEが立っている時に有効)
	"MotionTracking MK-II Plus",//	フィルタの名前
	TRACK_N,					//	トラックバーの数 (0なら名前初期値等もNULLでよい)
	track_name,					//	トラックバーの名前郡へのポインタ
	track_default,				//	トラックバーの初期値郡へのポインタ
	track_s, track_e,			//	トラックバーの数値の下限上限 (NULLなら全て0～256)
	CHECK_N,					//	チェックボックスの数 (0なら名前初期値等もNULLでよい)
	check_name,					//	チェックボックスの名前郡へのポインタ
	check_default,				//	チェックボックスの初期値郡へのポインタ
	func_proc,					//	フィルタ処理関数へのポインタ (NULLなら呼ばれません)
	NULL,						//	開始時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	NULL,						//	終了時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	NULL,						//	設定が変更されたときに呼ばれる関数へのポインタ (NULLなら呼ばれません)
	func_WndProc,						//	設定ウィンドウにウィンドウメッセージが来た時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	NULL, NULL,					//	システムで使いますので使用しないでください
	NULL,						//  拡張データ領域へのポインタ (FILTER_FLAG_EX_DATAが立っている時に有効)
	NULL,						//  拡張データサイズ (FILTER_FLAG_EX_DATAが立っている時に有効)
	verstr,
	//  フィルタ情報へのポインタ (FILTER_FLAG_EX_INFORMATIONが立っている時に有効)
	NULL,						//	セーブが開始される直前に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	NULL,						//	セーブが終了した直前に呼ばれる関数へのポインタ (NULLなら呼ばれません)
};
BOOL hsv_func_proc(FILTER *fp, FILTER_PROC_INFO *fpip);
FILTER_DLL filter_hsv = {
	FILTER_FLAG_EX_INFORMATION | FILTER_FLAG_RADIO_BUTTON,	//	フィルタのフラグ
	0, 0,						//	設定ウインドウのサイズ (FILTER_FLAG_WINDOW_SIZEが立っている時に有効)
	"Pre-track:HSV Cvt",			//	フィルタの名前
	NULL,					//	トラックバーの数 (0なら名前初期値等もNULLでよい)
	NULL,					//	トラックバーの名前郡へのポインタ
	NULL,				//	トラックバーの初期値郡へのポインタ
	NULL, NULL,			//	トラックバーの数値の下限上限 (NULLなら全て0～256)
	HSV_CHECK_N,					//	チェックボックスの数 (0なら名前初期値等もNULLでよい)
	hsv_check_name,					//	チェックボックスの名前郡へのポインタ
	hsv_check_default,				//	チェックボックスの初期値郡へのポインタ
	hsv_func_proc,					//	フィルタ処理関数へのポインタ (NULLなら呼ばれません)
	NULL,						//	開始時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	NULL,						//	終了時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	NULL,						//	設定が変更されたときに呼ばれる関数へのポインタ (NULLなら呼ばれません)
	NULL,						//	設定ウィンドウにウィンドウメッセージが来た時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	NULL, NULL,					//	システムで使いますので使用しないでください
	NULL,						//  拡張データ領域へのポインタ (FILTER_FLAG_EX_DATAが立っている時に有効)
	NULL,						//  拡張データサイズ (FILTER_FLAG_EX_DATAが立っている時に有効)
	"Pre-track:HSV Converstion",
	//  フィルタ情報へのポインタ (FILTER_FLAG_EX_INFORMATIONが立っている時に有効)
	NULL,						//	セーブが開始される直前に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	NULL,						//	セーブが終了した直前に呼ばれる関数へのポインタ (NULLなら呼ばれません)
};

BOOL bgs_func_proc(FILTER *fp, FILTER_PROC_INFO *fpip);
BOOL bgs_func_WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, void *editp, FILTER *fp);
FILTER_DLL filter_bgs = {
	FILTER_FLAG_EX_INFORMATION | FILTER_FLAG_RADIO_BUTTON,	//	フィルタのフラグ
	0, 0,						//	設定ウインドウのサイズ (FILTER_FLAG_WINDOW_SIZEが立っている時に有効)
	"Pre-track:BGSubtraction",			//	フィルタの名前
	BGS_TRACK_N,					//	トラックバーの数 (0なら名前初期値等もNULLでよい)
	bgs_track_name,					//	トラックバーの名前郡へのポインタ
	bgs_track_default,				//	トラックバーの初期値郡へのポインタ
	bgs_track_s, bgs_track_e,			//	トラックバーの数値の下限上限 (NULLなら全て0～256)
	BGS_CHECK_N,					//	チェックボックスの数 (0なら名前初期値等もNULLでよい)
	bgs_check_name,					//	チェックボックスの名前郡へのポインタ
	bgs_check_default,				//	チェックボックスの初期値郡へのポインタ
	bgs_func_proc,					//	フィルタ処理関数へのポインタ (NULLなら呼ばれません)
	NULL,						//	開始時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	NULL,						//	終了時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	NULL,						//	設定が変更されたときに呼ばれる関数へのポインタ (NULLなら呼ばれません)
	bgs_func_WndProc,						//	設定ウィンドウにウィンドウメッセージが来た時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	NULL, NULL,					//	システムで使いますので使用しないでください
	NULL,						//  拡張データ領域へのポインタ (FILTER_FLAG_EX_DATAが立っている時に有効)
	NULL,						//  拡張データサイズ (FILTER_FLAG_EX_DATAが立っている時に有効)
	"Pre-track:Background Subtraction",
	//  フィルタ情報へのポインタ (FILTER_FLAG_EX_INFORMATIONが立っている時に有効)
	NULL,						//	セーブが開始される直前に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	NULL,						//	セーブが終了した直前に呼ばれる関数へのポインタ (NULLなら呼ばれません)
};

static cv::Mat ocvImage;
static cv::Rect2d boundingBox;
static bool selectObj = false;
static bool startSel = false;
//static bool paused;
static int selA, selB;
static std::vector<cv::Rect2d> track_result;
static std::vector<bool> track_found;
typedef struct{
	void* editp;
	FILTER* fp;
}ParaCombo;

typedef struct{
	UINT32 frame;
	int cx;
	int cy;
	int width;
	int height;
	bool error;
}FRMFIX;

typedef struct{
	int start; //0-based frame index
	int end;
	int vi_start; //vector index
	int vi_end;
}FRMGROUP;



//---------------------------------------------------------------------
//		フィルタ構造体のポインタを渡す関数
//---------------------------------------------------------------------
FILTER_DLL* filterlist[] = { &filter, &filter_hsv, &filter_bgs, NULL };
EXTERN_C FILTER_DLL __declspec(dllexport) ** __stdcall GetFilterTableList(void)
{
	return (FILTER_DLL**)&filterlist;
}
//**************************************************************
//      Helper functions
//**************************************************************

// Exception filter
int Except_Eval(int code)
{
	switch (code)
	{
	case EXCEPTION_FLT_DENORMAL_OPERAND:
		MessageBox(NULL, "FLT_DENORMAL_OPERAND", "Error", MB_OK);
		return EXCEPTION_CONTINUE_EXECUTION;
		break;
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		MessageBox(NULL, "DIV_BY_ZERO", "Error", MB_OK);
		return EXCEPTION_CONTINUE_EXECUTION;
		break;
	case EXCEPTION_FLT_INEXACT_RESULT:
		MessageBox(NULL, "FLT_INEXACT_RESULT", "Error", MB_OK);
		return EXCEPTION_CONTINUE_EXECUTION;
		break;
	case EXCEPTION_FLT_INVALID_OPERATION:
		MessageBox(NULL, "FLT_INVALID_OP", "Error", MB_OK);
		return EXCEPTION_CONTINUE_EXECUTION;
		break;
	case EXCEPTION_FLT_OVERFLOW:
		MessageBox(NULL, "FLT_OVERFLOW", "Error", MB_OK);
		return EXCEPTION_CONTINUE_EXECUTION;
		break;
	case EXCEPTION_FLT_UNDERFLOW:
		MessageBox(NULL, "FLT_UNDERFLOW", "Error", MB_OK);
		return EXCEPTION_CONTINUE_EXECUTION;
		break;
	case EXCEPTION_FLT_STACK_CHECK:
		MessageBox(NULL, "FLT_STACK_CHECK", "Error", MB_OK);
		return EXCEPTION_CONTINUE_EXECUTION;
		break;
	default:
		MessageBox(NULL, "Non-FLT Exception", "Error", MB_OK);
		return ExceptionContinueExecution;
	}
}

// Mouse callback function for object selection
static void onMouse(int event, int x, int y, int, void*)
{
	switch (event)
	{
	case cv::EVENT_LBUTTONDOWN:
		//set origin of the bounding box
		startSel = true;
		selectObj = false;
		boundingBox.x = x;
		boundingBox.y = y;
		break;
	case cv::EVENT_LBUTTONUP:
		//set with and height of the bounding box
		boundingBox.width = std::abs(x - boundingBox.x);
		boundingBox.height = std::abs(y - boundingBox.y);
		if (x < boundingBox.x)
		{
			if (x > 0)
			{
				boundingBox.x = x;
			}
			else
			{
				boundingBox.x = 0;
			}
		}
		if (y < boundingBox.y)
		{
			if (y > 0)
			{
				boundingBox.y = y;
			}
			else
			{
				boundingBox.y = 0;
			}
		}
		selectObj = true;
		startSel = false;
		break;
	case cv::EVENT_MOUSEMOVE:

		if (startSel && !selectObj)
		{
			//draw the bounding box
			cv::Mat currentFrame;
			ocvImage.copyTo(currentFrame);
			cv::rectangle(currentFrame, cv::Point((int)boundingBox.x, (int)boundingBox.y), cv::Point(x, y), cv::Scalar(0, 255, 0), 1, 1);

			imshow("Object Selection", currentFrame);
		}
		break;
	}
}

//Callback function for trackbar used in result preview
static void onChange(int pos, void* userdata)
{
	ParaCombo* para = (ParaCombo*)userdata;
	void* editp = para->editp;
	FILTER* fp = para->fp;
	int srcw, srch;
	fp->exfunc->get_pixel_filtered(editp, selA + pos, NULL, &srcw, &srch);
	PIXEL* aubuf = new PIXEL[srcw*srch];
	fp->exfunc->get_pixel_filtered(editp, selA + pos, aubuf, NULL, NULL);
	cv::Mat disp(srch, srcw, CV_8UC3, aubuf);
	cv::flip(disp, disp, 0);
	if (track_found[pos])
	{
		cv::rectangle(disp, track_result[pos], cv::Scalar(255, 0, 0), 2, 1);
		cv::putText(disp, "OK", cv::Point(0, 50), cv::FONT_HERSHEY_PLAIN, 2.0, cv::Scalar(0, 255, 0), 2);
	}
	else
	{
		cv::putText(disp, "ERROR", cv::Point(0, 50), cv::FONT_HERSHEY_PLAIN, 2.0, cv::Scalar(0, 0, 255), 2);
	}
	
	cv::Mat disp_clone;
	disp.copyTo(disp_clone);
	disp.~Mat();
	delete[] aubuf;
	imshow("Tracking Result", disp_clone);
}


//Find single-frame error to be interpolate
//RETURN: a std::vector<UINT32> containing relevant index -> out_list
//RETURN: no. of inter-frame ->func return int
static int find_inter_frame(std::vector<bool> &err_list, std::vector<UINT32> &out_list)
{
	//TODO
	int loop_last_index = err_list.size() - 3;
	int interfrm_count = 0;
	if (err_list.size() < 3)
	{
		return FALSE;
	}
	out_list.clear();
	for (int i = 0; i <= loop_last_index; i++)
	{
		bool S, M, E;
		S = err_list[i];
		M = err_list[i + 1];
		E = err_list[i + 2];
		if ((S && E) && !M)
		{
			interfrm_count++;
			out_list.push_back((UINT32)i + 1);
		}
	}
	return interfrm_count;
}

//Get center coordinate from Rect2d
static cv::Point getCenter(cv::Rect2d &box)
{
	cv::Point buf(0, 0);
	buf.x = ((int)box.tl().x + (int)box.br().x) / 2;
	buf.y = ((int)box.tl().y + (int)box.br().y) / 2;
	return buf;
}
static cv::Point getCenter(cv::Rect &box) //overload for Rect2i
{
	cv::Point buf(0, 0);
	buf.x = (box.tl().x + box.br().x) / 2;
	buf.y = (box.tl().y + box.br().y) / 2;
	return buf;
}

//Interpolate frames, and transform to AviUtl coordinate
static void fix_frame(std::vector<cv::Rect2d> &rect_list, std::vector<bool> &err_list, std::vector<UINT32> &inter_list, std::vector<FRMFIX> &out, int frm_w, int frm_h)
{
	//TODO
	//Interpolation phase
	if (inter_list.size() > 0)
	{
		for (size_t f = 0; f < inter_list.size(); f++)
		{
			int v_idx = inter_list[f];
			int now_cx, now_cy, now_tlx, now_tly;
			int prevW, nowW, nextW;
			int prevH, nowH, nextH;

			cv::Point prevC(getCenter(rect_list[v_idx - 1]));
			prevW = (int)rect_list[v_idx - 1].width;
			prevH = (int)rect_list[v_idx - 1].height;

			cv::Point nextC(getCenter(rect_list[v_idx + 1]));
			nextW = (int)rect_list[v_idx + 1].width;
			nextH = (int)rect_list[v_idx + 1].height;

			nowW = (prevW + nextW) / 2;
			nowH = (prevH + nextH) / 2;

			now_cx = (prevC.x + nextC.x) / 2;
			now_cy = (prevC.y + nextC.y) / 2;

			now_tlx = now_cx - (nowW / 2);
			now_tly = now_cy - (nowH / 2);
			//Update box data
			rect_list[v_idx].x = now_tlx;
			rect_list[v_idx].y = now_tly;
			rect_list[v_idx].width = nowW;
			rect_list[v_idx].height = nowH;
			//Update error state
			err_list[v_idx] = true;
		}
		
	}
	//Transform to AviUtl coordiante
	int dX = frm_w / -2;
	int dY = frm_h / -2;
	for (size_t i = 0; i < rect_list.size(); i++)
	{
		FRMFIX buf;
		cv::Point center(getCenter(rect_list[i]));
		buf.cx = center.x + dX;
		buf.cy = center.y + dY;
		buf.width = (int)rect_list[i].width;
		buf.height = (int)rect_list[i].height;
		buf.frame = i + selA + 1;
		buf.error = err_list[i];
		out.push_back(buf); //store to output vector
	}
}

//Group into objects
static void groupObject(std::vector<FRMFIX> &fixedframes, std::vector<FRMGROUP> &out)
{
	//TODO
	std::vector<int> startpos;
	std::vector<int> endpos;
	bool prevstate = false;
	for (size_t i = 0; i < fixedframes.size(); i++)
	{
		bool currentstate = fixedframes[i].error;
		if (prevstate != currentstate) // a state change marking obj boundary
		{
			if (currentstate) //F->T = start
			{
				startpos.push_back(i);
			}
			else //T->F = end (prev frame)
			{
				endpos.push_back(i - 1);
			}
			
		}
		prevstate = currentstate;
	}
	//If endpos has 1 less item than startpos, add the last item back
	if (endpos.size() < startpos.size())
	{
		endpos.push_back(fixedframes.size() - 1);
	}
	//set output
	out.clear();
	if (startpos.size() > 0) //if there is at least 1 object
	{
		for (size_t i = 0; i < startpos.size(); i++)
		{
			FRMGROUP buf;
			buf.vi_start = startpos[i];
			buf.vi_end = endpos[i];
			buf.start = buf.vi_start + selA;
			buf.end = buf.vi_end + selA;
			out.push_back(buf);
		}
	}
}
//BOOL func_init(FILTER *fp);

//BOOL func_exit(FILTER *fp);
//BOOL func_update(FILTER *fp, int status);
BOOL func_WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, void *editp, FILTER *fp)
{
	if (fp->exfunc->is_filter_active(fp) && fp->exfunc->is_editing(editp))
	{
		switch (wparam)
		{
		case MID_FILTER_BUTTON: //Object selection
		{
			
			//MessageBox(NULL, "Btn 1", "DEBUG", MB_OK);
			
			int srcw, srch;
			if (!fp->exfunc->get_pixel_filtered(editp, selA, NULL, &srcw, &srch))
			{
				MessageBox(NULL, "Cannot get frame size", "AviUtl API Error", MB_OK);
				return FALSE;
			};
			//int selA, selB;
			fp->exfunc->get_select_frame(editp, &selA, &selB);
			PIXEL* aubgr = new PIXEL[srcw*srch];
			if (fp->exfunc->get_pixel_filtered(editp, selA, aubgr, NULL, NULL))
			{
				cv::Mat cvBuffer(srch, srcw, CV_8UC3, aubgr);
				cv::flip(cvBuffer, cvBuffer, 0);
				cvBuffer.copyTo(ocvImage);
				cvBuffer.~Mat();
				delete[] aubgr;
				cv::namedWindow("Object Selection", cv::WINDOW_AUTOSIZE);
				cv::setMouseCallback("Object Selection", onMouse, 0);
				cv::imshow("Object Selection", ocvImage);
			}
			return TRUE;
			break;
		}
		case MID_FILTER_BUTTON + 1: //Analyze
		{
			if (!selectObj)
			{
				MessageBox(NULL, "Nothing selected", "Operation Error", MB_OK);
				return FALSE;
			}

			track_result.clear();
			track_found.clear();
			ocvImage.empty();
			cv::Rect2d boxbackup = boundingBox;
			//Correct for out-of-bound box
			int frmw, frmh;
			if (!fp->exfunc->get_pixel_filtered(editp, selA, NULL, &frmw, &frmh))
			{
				MessageBox(NULL, "Cannot get frame size", "AviUtl API Error", MB_OK);
				return FALSE;
			}
			if (boundingBox.br().x > frmw)
			{
				boundingBox.width = frmw - boundingBox.x;
			}
			if (boundingBox.br().y > frmh)
			{
				boundingBox.height = frmh - boundingBox.y;
			}
			int64 start_time = cv::getTickCount();
			// Create Tracker
			cv::Ptr<cv::Tracker> tracker;
			switch (fp->track[0] - 1) {
			case 0:
				tracker = cv::TrackerBoosting::create();
				break;
			case 1:
				tracker = cv::TrackerMIL::create();
				break;
			case 2:
				tracker = cv::TrackerMedianFlow::create();
				break;
			case 3:
				tracker = cv::TrackerTLD::create();
				break;
			case 4:
				tracker = cv::TrackerKCF::create();
				break;
			case 5:
				tracker = cv::TrackerCSRT::create();
				break;
			default:
				tracker = cv::TrackerMOSSE::create();
				break;
			}
			if (tracker == NULL)
			{
				MessageBox(NULL, "Error when creating tracker", "OpenCV3 Error", MB_OK);
				return FALSE;
			}
			//cv::String trackWndTitle("Tracking");
			//cv::namedWindow(trackWndTitle, cv::WINDOW_AUTOSIZE);

			bool track_init = false;
			track_result.clear();
			track_found.clear();
			//Loop through selected frames for analysis
			TCHAR shortmsg[64] = { 0 };
			int64 prev_stamp, new_stamp;
			PIXEL* nextau = new PIXEL[frmw * frmh];
			for (int f = selA; f <= selB; f++)
			{
				//Set next frame
				if (fp->exfunc->get_pixel_filtered(editp, f, nextau, NULL, NULL))
				{
					cv::Mat cvNext(frmh, frmw, CV_8UC3, nextau);
					cv::flip(cvNext, cvNext, 0);
					cvNext.copyTo(ocvImage);
					cvNext.~Mat();
				}
				else
				{
					MessageBox(NULL, "Cannot get next image", "AviUtl API Error", MB_OK);
				}

				//sprintf_s(shortmsg, "%s processing frame %d / %d", track_method[fp->track[0]-1], f+1, selB+1);
				//SetWindowText(fp->hwnd, shortmsg);
				if (!track_init && selectObj)
				{
					//TODO
					SecureZeroMemory(shortmsg, sizeof(TCHAR[64]));
					sprintf_s(shortmsg, "%s tracker initializing...", track_method[fp->track[0] - 1]);
					SetWindowText(fp->hwnd, shortmsg);
					if (!tracker->init(ocvImage, boundingBox))
					{
						MessageBox(NULL, "Error initializing tracker", "OpenCV3 Error", MB_OK);
						delete[] nextau;
						return FALSE;
					}
					track_init = true;
					track_found.push_back(true);

					prev_stamp = cv::getTickCount();
				}
				else if (track_init)
				{
					new_stamp = cv::getTickCount();
					double fps = 1.0 / ((new_stamp - prev_stamp) / cv::getTickFrequency());
					SecureZeroMemory(shortmsg, sizeof(TCHAR[64]));
					sprintf_s(shortmsg, "%s processing frame %d/%d @%.2f fps", track_method[fp->track[0] - 1], f + 1, selB + 1, fps);
					SetWindowText(fp->hwnd, shortmsg);
					prev_stamp = cv::getTickCount();

					try {
						//cv::Rect2d bbox_backup = boundingBox;
						if (tracker->update(ocvImage, boundingBox))
						{
							track_found.push_back(true);
						}
						else
						{
							track_found.push_back(false);
							//boundingBox = bbox_backup;
						}
					}
					catch (...)
					{
						MessageBox(NULL, "Obscure tracker error", "Tracker update exception", MB_OK);
						delete[] nextau;
						return FALSE;
					}
				}
				if (boundingBox.area() <= 0)
				{
					boundingBox.width = 20;
					boundingBox.height = 20;
				}
				if (boundingBox.x < 0) {
					boundingBox.width += boundingBox.x;
					boundingBox.x = 0;
				}
				if (boundingBox.y < 0) {
					boundingBox.height += boundingBox.y;
					boundingBox.y = 0;
				}
				if (boundingBox.br().x > frmw)
				{
					boundingBox.width = frmw - boundingBox.x;
				}
				if (boundingBox.br().y > frmh)
				{
					boundingBox.height = frmh - boundingBox.y;
				}
				track_result.push_back(boundingBox);
			}
			delete[] nextau;
			int64 end_time = cv::getTickCount();
			double run_time = (end_time - start_time) / cv::getTickFrequency();
			SecureZeroMemory(shortmsg, sizeof(TCHAR[64]));
			sprintf_s(shortmsg, "%d frames tracked in %.2f sec", selB - selA, run_time);
			SetWindowText(fp->hwnd, shortmsg);
			SecureZeroMemory(shortmsg, sizeof(TCHAR[64]));
			sprintf_s(shortmsg, "Tracking Completed!\nAverage %.2f fps", (selB - selA) / run_time);
			MessageBox(NULL, shortmsg, "Tracking Completed!", MB_OK);
			boundingBox = boxbackup;
			return TRUE;

			break;
		}
		case MID_FILTER_BUTTON + 2: //View result
		{
			
			//MessageBox(NULL, "Btn 3", "DEBUG", MB_OK);
			if (track_result.size() > 0)
			{
				cv::namedWindow("Tracking Result", cv::WINDOW_AUTOSIZE);
				int pos_val = 0;
				int max_pos = selB - selA;
				ParaCombo parameters;
				parameters.editp = editp;
				parameters.fp = fp;
				cv::createTrackbar("Frame", "Tracking Result", &pos_val, max_pos, onChange, &parameters);
				onChange(pos_val, &parameters);
				cv::waitKey(0);
			}
			else
			{
				MessageBox(NULL, "No tracking result found", "Operational Error", MB_OK);
				return FALSE;
			}
			return TRUE;
			break;
		}
		case MID_FILTER_BUTTON +3: //Clear data
		{
			
			//MessageBox(NULL, "Btn 4", "DEBUG", MB_OK);
			selA = 0;
			selB = 0;
			boundingBox.x = 0;
			boundingBox.y = 0;
			boundingBox.width = 0;
			boundingBox.height = 0;
			track_result.clear();
			track_found.clear();
			startSel = false;
			selectObj = false;
			ocvImage.empty();
			SetWindowText(fp->hwnd, "MotionTracking MK-II Plus");
			MessageBox(NULL, "Selection states, results and image cache reseted", "INFO", MB_OK);
			return TRUE;
			break;
		}
		case MID_FILTER_BUTTON + 6: //Save EXO
		{
			//TODO
			//MessageBox(NULL, "Btn 5", "DEBUG", MB_OK);
			if (track_result.size() <= 0)
			{
				MessageBox(fp->hwnd, "No track data to save!", "Operation Error", MB_OK);
				return FALSE;
			}

			CHAR filename[MAX_PATH] = { 0 };
			if (fp->exfunc->dlg_get_save_name(filename, "ExEdit Object File(*.exo)\0*.exo;\0", "tracking.exo"))
			{
				//MessageBox(fp->hwnd, filename, "DEBUG", MB_OK);
				// Starts doing the real work
				std::ostringstream strbuf;
				TCHAR boilerplate[2048] = { 0 };
				TCHAR fmtstr[2048] = { 0 };
				/* Common Project Data*/
				int width, height;
				FILE_INFO fi;
				fp->exfunc->get_pixel_filtered(editp, selA, NULL, &width, &height);
				fp->exfunc->get_file_info(editp, &fi);
				/* End common prj data*/
				/* Format and write to buffer*/
				LoadString(fp->dll_hinst, IDS_PRJHEADER, boilerplate, 2048);
				sprintf_s(fmtstr, sizeof(TCHAR[2048]), boilerplate, width, height, fi.video_rate, fi.video_scale, fi.frame_n, fi.audio_rate);
				strbuf << fmtstr;
				SecureZeroMemory(boilerplate, sizeof(TCHAR[2048]));
				SecureZeroMemory(fmtstr, sizeof(TCHAR[2048]));
				//* END OF COMMON PROJECT HEADER*//
				//Starts object processing //
				/* vector storage for bounding-box interpolation and various checks*/
				std::vector<UINT32> frmInterpolate; //frames to be interpolate;
				std::vector<FRMFIX> fixedFrm; //interpolated and use AviUtl coordinate
				std::vector<FRMGROUP> gpinfo; //strats and ends of objects
				//
				find_inter_frame(track_found, frmInterpolate);
				fix_frame(track_result, track_found, frmInterpolate, fixedFrm, width, height);
				groupObject(fixedFrm, gpinfo);
				// Should now have all info we need...
				int object_id = 0; //keep track of how many segments/objects have been added
				for (size_t o = 0; o < gpinfo.size(); o++) //loop through each object
				{
					int oStart, oEnd;
					oStart = gpinfo[o].vi_start;
					oEnd = gpinfo[o].vi_end;
					bool firstframe = true;
					for (int f = oStart; f <= oEnd; f++) //each frames in this object
					{
						//Section Num
						char head_num[32];
						sprintf_s(head_num, sizeof(char[32]), "[%d]\n\0", object_id);
						strbuf << head_num;
						//Common obj param
						LoadString(fp->dll_hinst, IDS_OBJPARAM, boilerplate, 2048);
						sprintf_s(fmtstr, sizeof(TCHAR[2048]), boilerplate, fixedFrm[f].frame, fixedFrm[f].frame, 1);//st, ed, layer
						strbuf << fmtstr;
						SecureZeroMemory(boilerplate, sizeof(TCHAR[2048]));
						SecureZeroMemory(fmtstr, sizeof(TCHAR[2048]));
						//2 more param for Figure obj
						if (!fp->check[5])
						{
							LoadString(fp->dll_hinst, IDS_FIGUREPARAMA, boilerplate, 2048);
							strbuf << boilerplate; //verbatim copy
							SecureZeroMemory(boilerplate, sizeof(TCHAR[2048]));
						}
						if (!firstframe)
						{
							LoadString(fp->dll_hinst, IDS_OBJCHAIN, boilerplate, 2048);
							strbuf << boilerplate; //verbatim copy
							SecureZeroMemory(boilerplate, sizeof(TCHAR[2048]));
						}
						//Section [*.0] Graphics/Sub-filter
						sprintf_s(head_num, sizeof(char[32]), "[%d.0]\n\0", object_id);
						strbuf << head_num;
						//
						if (fp->check[5]) //Sub-filter
						{
							if (fp->check[4])//English EXO
							{
								LoadString(fp->dll_hinst, IDS_SFPARAM_EN, boilerplate, 2048);
							}
							else //JP
							{
								LoadString(fp->dll_hinst, IDS_SFPARAM_JP, boilerplate, 2048);//TODO: Set JP text
							}
							int Xi, Xf, Yi, Yf, size_st, size_ed;
							double  rAsp_st, rAsp_ed;
							Xi = fixedFrm[f].cx;
							Yi = fixedFrm[f].cy;
							if (fixedFrm[f].width > fixedFrm[f].height) //-ve rAsp, const width
							{
								size_st = fixedFrm[f].width;
								rAsp_st = -100.0 * (1.0 - ((double)fixedFrm[f].height / (double)fixedFrm[f].width));
							}
							else if (fixedFrm[f].width < fixedFrm[f].height) // +ve rAsp, const height
							{
								size_st = fixedFrm[f].height;
								rAsp_st = 100.0 * (1.0 - ((double)fixedFrm[f].width / (double)fixedFrm[f].height));
							}
							else // rAsp=0, square
							{
								size_st = fixedFrm[f].width;
								rAsp_st = 0.0;
							}
							if ((size_t)f >= (fixedFrm.size() - 1))//special handling for last frame
							{
								Xf = Xi;
								Yf = Yi;
								size_ed = size_st;
								rAsp_ed = rAsp_st;
							}
							else
							{
								Xf = fixedFrm[f + 1].cx;
								Yf = fixedFrm[f + 1].cy;
								if (fixedFrm[f + 1].width > fixedFrm[f + 1].height) //-ve rAsp, const width
								{
									size_ed = fixedFrm[f + 1].width;
									rAsp_ed = -100.0 * (1.0 - ((double)fixedFrm[f + 1].height / (double)fixedFrm[f + 1].width));
								}
								else if (fixedFrm[f + 1].width < fixedFrm[f + 1].height) // +ve rAsp, const height
								{
									size_ed = fixedFrm[f + 1].height;
									rAsp_ed = 100.0 * (1.0 - ((double)fixedFrm[f + 1].width / (double)fixedFrm[f + 1].height));
								}
								else // rAsp=0, square
								{
									size_ed = fixedFrm[f + 1].width;
									rAsp_ed = 0.0;
								}
							}

							sprintf_s(fmtstr, sizeof(TCHAR[2048]), boilerplate, (double)Xi, (double)Xf, (double)Yi, (double)Yf, size_st, size_ed, rAsp_st, rAsp_ed);//X-st, X-ed, Y-st, Y-ed, size_start, size_end, rAsp-st, rAsp-ed
							strbuf << fmtstr;
							SecureZeroMemory(boilerplate, sizeof(TCHAR[2048]));
							SecureZeroMemory(fmtstr, sizeof(TCHAR[2048]));
						}
						else //Graphics
						{
							if (fp->check[4])//English EXO
							{
								LoadString(fp->dll_hinst, IDS_FIGUREPARAMB_EN, boilerplate, 2048);
							}
							else //JP
							{
								LoadString(fp->dll_hinst, IDS_FIGUREPARAMB_JP, boilerplate, 2048);//TODO: Set JP text
							}
							strbuf << boilerplate;
							SecureZeroMemory(boilerplate, sizeof(TCHAR[2048]));

						}
						//Section [*.1] Resize FX or Mono FX
						sprintf_s(head_num, sizeof(char[32]), "[%d.1]\n\0", object_id);
						strbuf << head_num;
						//
						if (fp->check[5])// Mono FX for Sub-filter
						{
							if (fp->check[4])//English EXO
							{
								LoadString(fp->dll_hinst, IDS_FXMONO_EN, boilerplate, 2048);
							}
							else //JP
							{
								LoadString(fp->dll_hinst, IDS_FXMONO_JP, boilerplate, 2048);//TODO: Set JP text
							}
							strbuf << boilerplate;
							SecureZeroMemory(boilerplate, sizeof(TCHAR[2048]));
						}
						else //Resize FX for Graphics
						{
							int Wi, Wf, Hi, Hf;

							Wi = fixedFrm[f].width;
							Hi = fixedFrm[f].height;

							if ((size_t)f >= (fixedFrm.size() - 1)) //Last frame
							{
								Wf = Wi;
								Hf = Hi;
							}
							else // Normal
							{
								Wf = fixedFrm[f + 1].width;
								Hf = fixedFrm[f + 1].height;
							}

							if (fp->check[4])//English EXO
							{
								LoadString(fp->dll_hinst, IDS_FXRESIZE_EN, boilerplate, 2048);
							}
							else //JP
							{
								LoadString(fp->dll_hinst, IDS_FXRESIZE_JP, boilerplate, 2048);//TODO: Set JP text
							}
							sprintf_s(fmtstr, sizeof(TCHAR[2048]), boilerplate, (double)Wi, (double)Wf, (double)Hi, (double)Hf);
							strbuf << fmtstr;
							SecureZeroMemory(boilerplate, sizeof(TCHAR[2048]));
							SecureZeroMemory(fmtstr, sizeof(TCHAR[2048]));
						}
						//Section [*.2] Std Drawing for Graphics
						//Coordinate animation part for Graphics
						if (!fp->check[5]) //only for graphics
						{
							sprintf_s(head_num, sizeof(char[32]), "[%d.2]\n\0", object_id);
							strbuf << head_num;
							//
							int Xi, Xf, Yi, Yf;
							Xi = fixedFrm[f].cx;
							Yi = fixedFrm[f].cy;
							if ((size_t)f >= (fixedFrm.size() - 1))//last frame
							{
								Xf = Xi;
								Yf = Yi;
							}
							else //Normal
							{
								Xf = fixedFrm[f + 1].cx;
								Yf = fixedFrm[f + 1].cy;
							}
							if (fp->check[4])//English EXO
							{
								LoadString(fp->dll_hinst, IDS_STDDRAW_EN, boilerplate, 2048);
							}
							else //JP
							{
								LoadString(fp->dll_hinst, IDS_STDDRAW_JP, boilerplate, 2048);//TODO: Set JP text
							}
							sprintf_s(fmtstr, sizeof(TCHAR[2048]), boilerplate, (double)Xi, (double)Xf, (double)Yi, (double)Yf);
							strbuf << fmtstr;
							SecureZeroMemory(boilerplate, sizeof(TCHAR[2048]));
							SecureZeroMemory(fmtstr, sizeof(TCHAR[2048]));
						}
						//
						object_id++;
						firstframe = false;
					}
				}
				//Write to file
				std::ofstream fhandle(filename, std::ofstream::out | std::ofstream::trunc);
				if (fhandle.is_open())
				{
					fhandle << strbuf.str();
					fhandle.flush();
					fhandle.close();
					MessageBox(fp->hwnd, "DONE", "Finished", MB_OK);
				}
				else
				{
					MessageBox(fp->hwnd, "Cannot write to file!", "File I/O ERROR", MB_OK);
					return FALSE;
				}

			}

			return TRUE;
			break;
		}
		
		case MID_FILTER_BUTTON + 9: //Help button
		{
			MessageBoxEx(NULL, help_text, "Who... gonna to help you!", MB_OK | MB_ICONINFORMATION, MAKELANGID(LANG_JAPANESE, SUBLANG_JAPANESE_JAPAN));
			return TRUE;
			break;
		}
		default:
		{
			//TODO
			return FALSE;
		}
		}
	}
	
	return FALSE;
}

//For directly blurring
BOOL func_proc(FILTER *fp, FILTER_PROC_INFO *fpip)
{
	bool isFilterActive, isEditing, isFrameInRng, hasResult, redraw;
	redraw = false;
	isFilterActive = (fp->exfunc->is_filter_active(fp) == TRUE);
	isEditing = (fp->exfunc->is_editing(fpip->editp) == TRUE);
	hasResult = track_result.size() > fpip->frame - selA;
	isFrameInRng = (fpip->frame >= selA) && (fpip->frame <= selB);

	if (isFilterActive && isEditing && fp->check[7] && hasResult && isFrameInRng)
	{
		if (track_found[fpip->frame - selA])
		{
			size_t frmsize = fpip->w* fpip->h;
			PIXEL* aubuf = new PIXEL[frmsize];
			PIXEL_YC* yc_src = new PIXEL_YC[frmsize];
			byte* p1 = (byte*)fpip->ycp_edit;
			byte* p2 = (byte*)yc_src;
			size_t yc_linesize = sizeof(PIXEL_YC)*fpip->w;
			size_t yc_maxlinesize = sizeof(PIXEL_YC)*fpip->max_w;
			for (int line = 0; line < fpip->h; line++)
			{
				memcpy_s(p2, yc_linesize, p1, yc_linesize);
				p1 += yc_maxlinesize;
				p2 += yc_linesize;
			}
			fp->exfunc->yc2rgb(aubuf, yc_src, frmsize);
			//fp->exfunc->get_pixel_filtered(fpip->editp, fpip->frame, aubuf, NULL, NULL);

			cv::Mat ocvbuf(fpip->h, fpip->w, CV_8UC3, aubuf);
			//cv::flip(ocvbuf, ocvbuf, 0);
			cv::Mat blurArea(ocvbuf, track_result[fpip->frame - selA]);
			cv::blur(blurArea, blurArea, cv::Size(21, 21));
			PIXEL* aubuf2 = (PIXEL*)ocvbuf.data;
			PIXEL_YC* ycbuf = new PIXEL_YC[frmsize];
			fp->exfunc->rgb2yc(ycbuf, aubuf2, frmsize);
			size_t linesize = sizeof(PIXEL_YC)*fpip->w;
			size_t canvas_linesize = sizeof(PIXEL_YC)*fpip->max_w;
			byte* ptemp = (byte*)fpip->ycp_temp;
			byte* psrc = (byte*)ycbuf;
			for (int line = 0; line < fpip->h; line++)
			{
				memcpy_s(ptemp, canvas_linesize, psrc, linesize);
				ptemp += canvas_linesize;
				psrc += linesize;
			}
			//swap ptr
			PIXEL_YC* ycswap = fpip->ycp_temp;
			fpip->ycp_temp = fpip->ycp_edit;
			fpip->ycp_edit = ycswap;
			//Cleanup

			delete[] ycbuf;
			blurArea.~Mat();
			ocvbuf.~Mat();
			delete[] aubuf;
			delete[] yc_src;
			redraw = true;
		}
	}
	if (fp->check[8] && isFilterActive && isEditing)
	{
		//AviUtl -> OCV
		size_t frmsize = fpip->w* fpip->h;
		PIXEL* aubuf = new PIXEL[frmsize];
		PIXEL_YC* yc_src = new PIXEL_YC[frmsize];
		byte* p1 = (byte*)fpip->ycp_edit;
		byte* p2 = (byte*)yc_src;
		size_t yc_linesize = sizeof(PIXEL_YC)*fpip->w;
		size_t yc_maxlinesize = sizeof(PIXEL_YC)*fpip->max_w;
		for (int line = 0; line < fpip->h; line++)
		{
			memcpy_s(p2, yc_linesize, p1, yc_linesize);
			p1 += yc_maxlinesize;
			p2 += yc_linesize;
		}
		fp->exfunc->yc2rgb(aubuf, yc_src, frmsize);
		cv::Mat ocvbuf(fpip->h, fpip->w, CV_8UC3, aubuf);
		//
		//Convert to greyscale
		cv::Mat ocvGrey;
		cv::cvtColor(ocvbuf, ocvGrey, cv::COLOR_BGR2GRAY, 1);
		//Haar part
		//Check if XML exists in root or plugin folder
		cv::CascadeClassifier frontface = cv::CascadeClassifier::CascadeClassifier();
		cv::CascadeClassifier profileface = cv::CascadeClassifier::CascadeClassifier();
		if (!frontface.load("haarcascade_frontalface_default.xml"))
		{
			frontface.load("./plugins/haarcascade_frontalface_default.xml");

		}
		if (!profileface.load("haarcascade_profileface.xml"))
		{
			profileface.load("./plugins/haarcascade_profileface.xml");
		}
		//if XML loaded, proceed
		if (!frontface.empty() && !profileface.empty())
		{
			std::vector<cv::Rect> frontface_list;
			std::vector<cv::Rect> profileface_list;
			frontface.detectMultiScale(ocvGrey, frontface_list, 1.1);
			profileface.detectMultiScale(ocvGrey, profileface_list, 1.1);
			//Loop through each result and apply blur
			if (frontface_list.size() > 0)
			{
				for (size_t i = 0; i < frontface_list.size(); i++)
				{
					cv::Mat toblur(ocvbuf, frontface_list[i]);
					int kwidth = frontface_list[i].width / 4;
					int kheight = frontface_list[i].height / 4;
					cv::blur(toblur, toblur, cv::Size(kwidth, kheight));
				}
			}
			if (profileface_list.size()>0)
			{
				for (size_t i = 0; i < profileface_list.size(); i++)
				{
					int kwidth = profileface_list[i].width / 4;
					int kheight = profileface_list[i].height / 4;
					cv::Mat toblur(ocvbuf, profileface_list[i]);
					cv::blur(toblur, toblur, cv::Size(kwidth, kheight));
				}
			}
			//Send back to aviutl if detected any face
			if (frontface_list.size() || profileface_list.size())
			{
				PIXEL* aubuf2 = (PIXEL*)ocvbuf.data;
				PIXEL_YC* ycbuf = new PIXEL_YC[frmsize];
				fp->exfunc->rgb2yc(ycbuf, aubuf2, frmsize);
				size_t linesize = sizeof(PIXEL_YC)*fpip->w;
				size_t canvas_linesize = sizeof(PIXEL_YC)*fpip->max_w;
				byte* ptemp = (byte*)fpip->ycp_temp;
				byte* psrc = (byte*)ycbuf;
				for (int line = 0; line < fpip->h; line++)
				{
					memcpy_s(ptemp, canvas_linesize, psrc, linesize);
					ptemp += canvas_linesize;
					psrc += linesize;
				}
				//swap ptr
				PIXEL_YC* ycswap = fpip->ycp_temp;
				fpip->ycp_temp = fpip->ycp_edit;
				fpip->ycp_edit = ycswap;
				delete[] ycbuf;
			}
			
			//Clean up
						
			ocvbuf.~Mat();
			delete[] aubuf;
			delete[] yc_src;
			redraw = true;
		}
	}
	return redraw;
}

//HSV Conversion filter
BOOL hsv_func_proc(FILTER *fp, FILTER_PROC_INFO *fpip)
{
	if (!(fp->exfunc->is_editing(fpip->editp) && fp->exfunc->is_filter_active(fp)))
	{
		return FALSE;
	}
	int w, h;
	w = fpip->w;
	h = fpip->h;
	PIXEL_YC* ycbuf = new PIXEL_YC[w*h];
	PIXEL* bgrbuf = new PIXEL[w*h];
	byte *src_ptr, *dst_ptr;
	src_ptr = (byte*)fpip->ycp_edit;
	dst_ptr = (byte*)ycbuf;
	size_t src_linesize = fpip->max_w * sizeof(PIXEL_YC);
	size_t dst_linesize = w * sizeof(PIXEL_YC);
	for (int line = 0; line < h; line++)
	{
		memcpy(dst_ptr, src_ptr, dst_linesize);
		dst_ptr += dst_linesize;
		src_ptr += src_linesize;
	}
	fp->exfunc->yc2rgb(bgrbuf, ycbuf, w*h);
	cv::Mat ocvImg(h, w, CV_8UC3, bgrbuf);
	cv::Mat outImg;
	cv::cvtColor(ocvImg, outImg, cv::COLOR_BGR2HSV_FULL);
	if (fp->check[1]) //Hue
	{
		std::vector<cv::Mat> channels;
		cv::split(outImg, channels);
		cv::cvtColor(channels[0], outImg, cv::COLOR_GRAY2BGR);
		channels.clear();
	}
	else if (fp->check[2]) //Sat
	{
		std::vector<cv::Mat> channels;
		cv::split(outImg, channels);
		cv::cvtColor(channels[1], outImg, cv::COLOR_GRAY2BGR);
		channels.clear();
	}
	else if (fp->check[3]) //Value
	{
		std::vector<cv::Mat> channels;
		cv::split(outImg, channels);
		cv::cvtColor(channels[2], outImg, cv::COLOR_GRAY2BGR);
		channels.clear();
	}
	fp->exfunc->rgb2yc(ycbuf, (PIXEL*)outImg.data, w*h);
	src_ptr = (byte*)ycbuf;
	dst_ptr = (byte*)fpip->ycp_temp;
	src_linesize = w* sizeof(PIXEL_YC);
	dst_linesize = fpip->max_w* sizeof(PIXEL_YC);
	for (int line = 0; line < h; line++)
	{
		memcpy(dst_ptr, src_ptr, src_linesize);
		src_ptr += src_linesize;
		dst_ptr += dst_linesize;
	}
	PIXEL_YC* temp = fpip->ycp_edit;
	fpip->ycp_edit = fpip->ycp_temp;
	fpip->ycp_temp = temp;
	outImg.~Mat();
	ocvImg.~Mat();
	delete[] bgrbuf;
	delete[] ycbuf;
	return TRUE;
}

//MOG2 and KNN Background subtraction
BOOL bgs_func_proc(FILTER *fp, FILTER_PROC_INFO *fpip)
{
	if (!(fp->exfunc->is_editing(fpip->editp) && fp->exfunc->is_filter_active(fp)))
	{
		return FALSE;
	}
	//
	if (!fp->exfunc->set_ycp_filtering_cache_size(fp, fpip->w, fpip->h, fp->track[0], NULL))
	{
		MessageBox(NULL, "Faile to set YCP Cache", "AviUtl API error", MB_OK);
		return FALSE;
	}
	bool isShadow = false;
	if (fp->track[1] > 0)
	{
		isShadow = true;
	}
	//boundary correction
	int frame_s, frame_e;
	frame_s = fpip->frame - fp->track[0];
	if (frame_s < 0) frame_s = 0;
	frame_e = fpip->frame + fp->track[0];
	if (frame_e > fpip->frame_n - 1) frame_e = fpip->frame_n - 1;
	//Setup OCV subtracttor
	cv::Mat mask;
	int w, h;
	if (fp->check[0])
	{
		cv::Ptr<cv::BackgroundSubtractorMOG2> mog2 = cv::createBackgroundSubtractorMOG2();
		mog2->setDetectShadows(isShadow);
		mog2->setHistory(fp->track[0]);
		mog2->setNMixtures(fp->track[2]);
		mog2->setBackgroundRatio((double)fp->track[3] / 100.0);
		for (int f = frame_s; f <= fpip->frame-1; f++)// Scan all frames in range
		{
			PIXEL_YC* ycbuf = fp->exfunc->get_ycp_filtering_cache_ex(fp, fpip->editp, f, &w, &h);
			PIXEL* bgrbuf = new PIXEL[w*h];
			fp->exfunc->yc2rgb(bgrbuf, ycbuf, w*h);
			cv::Mat ocvbuf(h, w, CV_8UC3, bgrbuf);
			mog2->apply(ocvbuf, mask);
			ocvbuf.~Mat();
			delete[] bgrbuf;
		}
		PIXEL_YC* ycbuf = fp->exfunc->get_ycp_filtering_cache_ex(fp, fpip->editp, fpip->frame, &w, &h);
		PIXEL* bgrbuf = new PIXEL[w*h];
		fp->exfunc->yc2rgb(bgrbuf, ycbuf, w*h);
		cv::Mat ocvbuf(h, w, CV_8UC3, bgrbuf);
		mog2->apply(ocvbuf, mask);
		cv::Mat element = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
		cv::erode(mask, mask, element);
		cv::dilate(mask, mask, element);
		cv::Mat tempbuf;
		ocvbuf.copyTo(tempbuf, mask);
				
		PIXEL_YC* ycout = new PIXEL_YC[w*h];
		fp->exfunc->rgb2yc(ycout, (PIXEL*)tempbuf.data, w*h);
		tempbuf.~Mat();
		mask.~Mat();
		byte* src_ptr = (byte*)ycout;
		byte* dst_ptr = (byte*)fpip->ycp_temp;
		size_t src_linesize = w* sizeof(PIXEL_YC);
		size_t dst_linesize = fpip->max_w * sizeof(PIXEL_YC);
		for (int line = 0; line < h; line++)
		{
			memcpy(dst_ptr, src_ptr, src_linesize);
			src_ptr += src_linesize;
			dst_ptr += dst_linesize;
		}
		PIXEL_YC* swap = fpip->ycp_edit;
		fpip->ycp_edit = fpip->ycp_temp;
		fpip->ycp_temp = swap;
		//Cleanup
		ocvbuf.~Mat();
		delete[] ycout;
		delete[] bgrbuf;
		return TRUE;
	}

	//KNN
	if (fp->check[1])
	{
		cv::Ptr<cv::BackgroundSubtractorKNN> knn = cv::createBackgroundSubtractorKNN();
		knn->setDetectShadows(isShadow);
		knn->setHistory(fp->track[0]*2);
		knn->setDist2Threshold(fp->track[4]);
		for (int f = frame_s; f <= frame_e; f++)// Scan all frames in range
		{
			PIXEL_YC* ycbuf = fp->exfunc->get_ycp_filtering_cache_ex(fp, fpip->editp, f, &w, &h);
			PIXEL* bgrbuf = new PIXEL[w*h];
			fp->exfunc->yc2rgb(bgrbuf, ycbuf, w*h);
			cv::Mat ocvbuf(h, w, CV_8UC3, bgrbuf);
			knn->apply(ocvbuf, mask);
			ocvbuf.~Mat();
			delete[] bgrbuf;
		}
		PIXEL_YC* ycbuf = fp->exfunc->get_ycp_filtering_cache_ex(fp, fpip->editp, fpip->frame, &w, &h);
		PIXEL* bgrbuf = new PIXEL[w*h];
		fp->exfunc->yc2rgb(bgrbuf, ycbuf, w*h);
		cv::Mat ocvbuf(h, w, CV_8UC3, bgrbuf);
		knn->apply(ocvbuf, mask);
		cv::Mat element = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
		cv::erode(mask, mask, element);
		cv::dilate(mask, mask, element);
		cv::Mat tempbuf;
		ocvbuf.copyTo(tempbuf, mask);

		PIXEL_YC* ycout = new PIXEL_YC[w*h];
		fp->exfunc->rgb2yc(ycout, (PIXEL*)tempbuf.data, w*h);
		tempbuf.~Mat();
		mask.~Mat();
		byte* src_ptr = (byte*)ycout;
		byte* dst_ptr = (byte*)fpip->ycp_temp;
		size_t src_linesize = w* sizeof(PIXEL_YC);
		size_t dst_linesize = fpip->max_w * sizeof(PIXEL_YC);
		for (int line = 0; line < h; line++)
		{
			memcpy(dst_ptr, src_ptr, src_linesize);
			src_ptr += src_linesize;
			dst_ptr += dst_linesize;
		}
		PIXEL_YC* swap = fpip->ycp_edit;
		fpip->ycp_edit = fpip->ycp_temp;
		fpip->ycp_temp = swap;
		//Cleanup
		ocvbuf.~Mat();
		delete[] ycout;
		delete[] bgrbuf;
		return TRUE;
	}

	//Mog-Mask only
	if (fp->check[2])
	{
		cv::Ptr<cv::BackgroundSubtractorMOG2> mog2 = cv::createBackgroundSubtractorMOG2();
		mog2->setDetectShadows(isShadow);
		mog2->setHistory(fp->track[0]);
		mog2->setNMixtures(fp->track[2]);
		mog2->setBackgroundRatio((double)fp->track[3] / 100.0);
		
		for (int f = frame_s; f <= fpip->frame - 1; f++)// Scan all frames in range
		{
			PIXEL_YC* ycbuf = fp->exfunc->get_ycp_filtering_cache_ex(fp, fpip->editp, f, &w, &h);
			PIXEL* bgrbuf = new PIXEL[w*h];
			fp->exfunc->yc2rgb(bgrbuf, ycbuf, w*h);
			cv::Mat ocvbuf(h, w, CV_8UC3, bgrbuf);
			mog2->apply(ocvbuf, mask);
			ocvbuf.~Mat();
			delete[] bgrbuf;
		}
		PIXEL_YC* ycbuf = fp->exfunc->get_ycp_filtering_cache_ex(fp, fpip->editp, fpip->frame, &w, &h);
		PIXEL* bgrbuf = new PIXEL[w*h];
		fp->exfunc->yc2rgb(bgrbuf, ycbuf, w*h);
		cv::Mat ocvbuf(h, w, CV_8UC3, bgrbuf);
		mog2->apply(ocvbuf, mask);
		cv::Mat element = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
		cv::erode(mask, mask, element);
		cv::dilate(mask, mask, element);
		cv::Mat tempbuf;
		cv::cvtColor(mask, tempbuf, cv::COLOR_GRAY2BGR);

		PIXEL_YC* ycout = new PIXEL_YC[w*h];
		fp->exfunc->rgb2yc(ycout, (PIXEL*)tempbuf.data, w*h);
		tempbuf.~Mat();
		mask.~Mat();
		byte* src_ptr = (byte*)ycout;
		byte* dst_ptr = (byte*)fpip->ycp_temp;
		size_t src_linesize = w* sizeof(PIXEL_YC);
		size_t dst_linesize = fpip->max_w * sizeof(PIXEL_YC);
		for (int line = 0; line < h; line++)
		{
			memcpy(dst_ptr, src_ptr, src_linesize);
			src_ptr += src_linesize;
			dst_ptr += dst_linesize;
		}
		PIXEL_YC* swap = fpip->ycp_edit;
		fpip->ycp_edit = fpip->ycp_temp;
		fpip->ycp_temp = swap;
		//Cleanup
		ocvbuf.~Mat();
		delete[] ycout;
		delete[] bgrbuf;
		return TRUE;
	}
	
	//KNN mask only
	if (fp->check[3])
	{
		cv::Ptr<cv::BackgroundSubtractorKNN> knn = cv::createBackgroundSubtractorKNN();
		knn->setDetectShadows(isShadow);
		knn->setHistory(fp->track[0] * 2);
		knn->setDist2Threshold(fp->track[4]);
		for (int f = frame_s; f <= frame_e; f++)// Scan all frames in range
		{
			PIXEL_YC* ycbuf = fp->exfunc->get_ycp_filtering_cache_ex(fp, fpip->editp, f, &w, &h);
			PIXEL* bgrbuf = new PIXEL[w*h];
			fp->exfunc->yc2rgb(bgrbuf, ycbuf, w*h);
			cv::Mat ocvbuf(h, w, CV_8UC3, bgrbuf);
			knn->apply(ocvbuf, mask);
			ocvbuf.~Mat();
			delete[] bgrbuf;
		}
		PIXEL_YC* ycbuf = fp->exfunc->get_ycp_filtering_cache_ex(fp, fpip->editp, fpip->frame, &w, &h);
		PIXEL* bgrbuf = new PIXEL[w*h];
		fp->exfunc->yc2rgb(bgrbuf, ycbuf, w*h);
		cv::Mat ocvbuf(h, w, CV_8UC3, bgrbuf);
		knn->apply(ocvbuf, mask);
		cv::Mat element = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
		cv::erode(mask, mask, element);
		cv::dilate(mask, mask, element);
		cv::Mat tempbuf;
		cv::cvtColor(mask, tempbuf, cv::COLOR_GRAY2BGR);

		PIXEL_YC* ycout = new PIXEL_YC[w*h];
		fp->exfunc->rgb2yc(ycout, (PIXEL*)tempbuf.data, w*h);
		tempbuf.~Mat();
		mask.~Mat();
		byte* src_ptr = (byte*)ycout;
		byte* dst_ptr = (byte*)fpip->ycp_temp;
		size_t src_linesize = w* sizeof(PIXEL_YC);
		size_t dst_linesize = fpip->max_w * sizeof(PIXEL_YC);
		for (int line = 0; line < h; line++)
		{
			memcpy(dst_ptr, src_ptr, src_linesize);
			src_ptr += src_linesize;
			dst_ptr += dst_linesize;
		}
		PIXEL_YC* swap = fpip->ycp_edit;
		fpip->ycp_edit = fpip->ycp_temp;
		fpip->ycp_temp = swap;
		//Cleanup
		ocvbuf.~Mat();
		delete[] ycout;
		delete[] bgrbuf;
		return TRUE;
	}
	return FALSE;
}

BOOL bgs_func_WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, void *editp, FILTER *fp)
{
	if (wparam == MID_FILTER_BUTTON + 4)
	{
		MessageBox(hwnd, bg_help, "INFO", MB_OK);
		return FALSE;
	}
	return FALSE;
}