#include <windows.h>
#include "filter.h"
#include "main.h"
#include "opencv2/opencv.hpp"


constexpr TCHAR *hsv_check_name[] = { "HSV", "Hue only", "Saturation only", "Value only" };
constexpr int   hsv_check_default[] = { 1, 0, 0, 0 };
constexpr int HSV_CHECK_N = sizeof(hsv_check_name) / sizeof(TCHAR*);

static_assert(HSV_CHECK_N == sizeof(hsv_check_default) / sizeof(int), "size of hsv_check_default mismatch with HSV_CHECK_N");

BOOL hsv_func_proc(FILTER *fp, FILTER_PROC_INFO *fpip);
FILTER_DLL filter_hsv = {
	FILTER_FLAG_EX_INFORMATION | FILTER_FLAG_RADIO_BUTTON,	//	フィルタのフラグ
	0, 0,									//	設定ウインドウのサイズ (FILTER_FLAG_WINDOW_SIZEが立っている時に有効)
	"Pre-track:HSV Cvt",					//	フィルタの名前
	NULL,									//	トラックバーの数 (0なら名前初期値等もNULLでよい)
	NULL,									//	トラックバーの名前郡へのポインタ
	NULL,									//	トラックバーの初期値郡へのポインタ
	NULL, NULL,								//	トラックバーの数値の下限上限 (NULLなら全て0～256)
	HSV_CHECK_N,							//	チェックボックスの数 (0なら名前初期値等もNULLでよい)
	const_cast<TCHAR **>(hsv_check_name),	//	チェックボックスの名前郡へのポインタ
	const_cast<int *>(hsv_check_default),	//	チェックボックスの初期値郡へのポインタ
	hsv_func_proc,							//	フィルタ処理関数へのポインタ (NULLなら呼ばれません)
	NULL,									//	開始時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	NULL,									//	終了時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	NULL,									//	設定が変更されたときに呼ばれる関数へのポインタ (NULLなら呼ばれません)
	NULL,									//	設定ウィンドウにウィンドウメッセージが来た時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	NULL, NULL,								//	システムで使いますので使用しないでください
	NULL,									//  拡張データ領域へのポインタ (FILTER_FLAG_EX_DATAが立っている時に有効)
	NULL,									//  拡張データサイズ (FILTER_FLAG_EX_DATAが立っている時に有効)
	"Pre-track:HSV Converstion",
	//  フィルタ情報へのポインタ (FILTER_FLAG_EX_INFORMATIONが立っている時に有効)
	NULL,									//	セーブが開始される直前に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	NULL,									//	セーブが終了した直前に呼ばれる関数へのポインタ (NULLなら呼ばれません)
};

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
	std::unique_ptr<PIXEL_YC[]> ycbuf = std::make_unique<PIXEL_YC[]>(w * h);
	std::unique_ptr<PIXEL[]> bgrbuf = std::make_unique<PIXEL[]>(w * h);
	byte *src_ptr, *dst_ptr;
	src_ptr = (byte*)fpip->ycp_edit;
	dst_ptr = (byte*)ycbuf.get();
	size_t src_linesize = fpip->max_w * sizeof(PIXEL_YC);
	size_t dst_linesize = w * sizeof(PIXEL_YC);
	for (int line = 0; line < h; line++)
	{
		memcpy(dst_ptr, src_ptr, dst_linesize);
		dst_ptr += dst_linesize;
		src_ptr += src_linesize;
	}
	fp->exfunc->yc2rgb(bgrbuf.get(), ycbuf.get(), w * h);
	cv::Mat ocvImg(h, w, CV_8UC3, bgrbuf.get());
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
	fp->exfunc->rgb2yc(ycbuf.get(), (PIXEL*)outImg.data, w * h);
	src_ptr = (byte*)ycbuf.get();
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
	return TRUE;
}
