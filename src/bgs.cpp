#include <windows.h>
#include "filter.h"
#include "main.h"
#include "opencv2/opencv.hpp"

#define BGS_CHECK_N 4
TCHAR *bgs_check_name[] = { "MOG2", "KNN","MOG2(Mask)", "KNN(Mask)" };
int bgs_check_default[] = { 0, 0, 0, 1, -1 };

#define BGS_TRACK_N 5
TCHAR *bgs_track_name[] = { "Range", "Shadow","NMix", "BG%", "d2T" };
int bgs_track_default[] = { 30, 0, 5, 70, 400 };
int bgs_track_s[] = { 1, 0, 1, 1, 100 };
int bgs_track_e[] = { 250, 1, 30, 99, 1000 };

BOOL bgs_func_proc(FILTER *fp, FILTER_PROC_INFO *fpip);
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
	NULL,						//	設定ウィンドウにウィンドウメッセージが来た時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	NULL, NULL,					//	システムで使いますので使用しないでください
	NULL,						//  拡張データ領域へのポインタ (FILTER_FLAG_EX_DATAが立っている時に有効)
	NULL,						//  拡張データサイズ (FILTER_FLAG_EX_DATAが立っている時に有効)
	"Pre-track:Background Subtraction",
	//  フィルタ情報へのポインタ (FILTER_FLAG_EX_INFORMATIONが立っている時に有効)
	NULL,						//	セーブが開始される直前に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	NULL,						//	セーブが終了した直前に呼ばれる関数へのポインタ (NULLなら呼ばれません)
};


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
			std::unique_ptr<PIXEL[]> bgrbuf = std::make_unique<PIXEL[]>(w * h);
			fp->exfunc->yc2rgb(bgrbuf.get(), ycbuf, w * h);
			cv::Mat ocvbuf(h, w, CV_8UC3, bgrbuf.get());
			mog2->apply(ocvbuf, mask);
		}
		PIXEL_YC* ycbuf = fp->exfunc->get_ycp_filtering_cache_ex(fp, fpip->editp, fpip->frame, &w, &h);
		std::unique_ptr<PIXEL[]> bgrbuf = std::make_unique<PIXEL[]>(w * h);
		fp->exfunc->yc2rgb(bgrbuf.get(), ycbuf, w * h);
		cv::Mat ocvbuf(h, w, CV_8UC3, bgrbuf.get());
		mog2->apply(ocvbuf, mask);
		cv::Mat element = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
		cv::erode(mask, mask, element);
		cv::dilate(mask, mask, element);
		cv::Mat tempbuf;
		ocvbuf.copyTo(tempbuf, mask);
				
		std::unique_ptr<PIXEL_YC[]> ycout = std::make_unique<PIXEL_YC[]>(w * h);
		fp->exfunc->rgb2yc(ycout.get(), (PIXEL*)tempbuf.data, w * h);
		byte* src_ptr = (byte*)ycout.get();
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
			std::unique_ptr<PIXEL[]> bgrbuf = std::make_unique<PIXEL[]>(w * h);
			fp->exfunc->yc2rgb(bgrbuf.get(), ycbuf, w * h);
			cv::Mat ocvbuf(h, w, CV_8UC3, bgrbuf.get());
			knn->apply(ocvbuf, mask);
		}
		PIXEL_YC* ycbuf = fp->exfunc->get_ycp_filtering_cache_ex(fp, fpip->editp, fpip->frame, &w, &h);
		std::unique_ptr<PIXEL[]> bgrbuf = std::make_unique<PIXEL[]>(w * h);
		fp->exfunc->yc2rgb(bgrbuf.get(), ycbuf, w * h);
		cv::Mat ocvbuf(h, w, CV_8UC3, bgrbuf.get());
		knn->apply(ocvbuf, mask);
		cv::Mat element = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
		cv::erode(mask, mask, element);
		cv::dilate(mask, mask, element);
		cv::Mat tempbuf;
		ocvbuf.copyTo(tempbuf, mask);

		std::unique_ptr<PIXEL_YC[]> ycout = std::make_unique<PIXEL_YC[]>(w * h);
		fp->exfunc->rgb2yc(ycout.get(), (PIXEL*)tempbuf.data, w * h);
		byte* src_ptr = (byte*)ycout.get();
		byte* dst_ptr = (byte*)fpip->ycp_temp;
		size_t src_linesize = w * sizeof(PIXEL_YC);
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
			std::unique_ptr<PIXEL[]> bgrbuf = std::make_unique<PIXEL[]>(w * h);
			fp->exfunc->yc2rgb(bgrbuf.get(), ycbuf, w* h);
			cv::Mat ocvbuf(h, w, CV_8UC3, bgrbuf.get());
			mog2->apply(ocvbuf, mask);
		}
		PIXEL_YC* ycbuf = fp->exfunc->get_ycp_filtering_cache_ex(fp, fpip->editp, fpip->frame, &w, &h);
		std::unique_ptr<PIXEL[]> bgrbuf = std::make_unique<PIXEL[]>(w * h);
		fp->exfunc->yc2rgb(bgrbuf.get(), ycbuf, w* h);
		cv::Mat ocvbuf(h, w, CV_8UC3, bgrbuf.get());
		mog2->apply(ocvbuf, mask);
		cv::Mat element = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
		cv::erode(mask, mask, element);
		cv::dilate(mask, mask, element);
		cv::Mat tempbuf;
		cv::cvtColor(mask, tempbuf, cv::COLOR_GRAY2BGR);

		std::unique_ptr<PIXEL_YC[]> ycout = std::make_unique<PIXEL_YC[]>(w * h);
		fp->exfunc->rgb2yc(ycout.get(), (PIXEL*)tempbuf.data, w* h);
		byte* src_ptr = (byte*)ycout.get();
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
			std::unique_ptr<PIXEL[]> bgrbuf = std::make_unique<PIXEL[]>(w * h);
			fp->exfunc->yc2rgb(bgrbuf.get(), ycbuf, w * h);
			cv::Mat ocvbuf(h, w, CV_8UC3, bgrbuf.get());
			knn->apply(ocvbuf, mask);
		}
		PIXEL_YC* ycbuf = fp->exfunc->get_ycp_filtering_cache_ex(fp, fpip->editp, fpip->frame, &w, &h);
		std::unique_ptr<PIXEL[]> bgrbuf = std::make_unique<PIXEL[]>(w * h);
		fp->exfunc->yc2rgb(bgrbuf.get(), ycbuf, w* h);
		cv::Mat ocvbuf(h, w, CV_8UC3, bgrbuf.get());
		knn->apply(ocvbuf, mask);
		cv::Mat element = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
		cv::erode(mask, mask, element);
		cv::dilate(mask, mask, element);
		cv::Mat tempbuf;
		cv::cvtColor(mask, tempbuf, cv::COLOR_GRAY2BGR);

		std::unique_ptr<PIXEL_YC[]> ycout = std::make_unique<PIXEL_YC[]>(w * h);
		fp->exfunc->rgb2yc(ycout.get(), (PIXEL*)tempbuf.data, w* h);
		byte* src_ptr = (byte*)ycout.get();
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
		return TRUE;
	}
	return FALSE;
}
