#include <Windows.h>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include "filter.h"
#include "opencv2/core/utility.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/tracking.hpp"
#include "opencv2/objdetect.hpp"
#include "opencv2/video.hpp"
#include "opencv2/video/tracking.hpp"
#include "resource.h"
#include "hsv.h"
#include "bgs.h"

HINSTANCE hModuleDLL = nullptr;


constexpr TCHAR* track_method[] = { "MIL", "KCF", "CSRT", "GOTURN", "DaSiamRPN", "Nano", "Vit"};
constexpr int METHOD_N = sizeof(track_method) / sizeof(TCHAR*);

constexpr TCHAR* track_name[] = { "Method", "Rect Hue" };   // トラックバーの名前
constexpr int    track_default[] = { 3, 120 };       // トラックバーの初期値
constexpr int    track_s[] = { 1, 0 };             // トラックバーの下限値
constexpr int    track_e[] = { METHOD_N, 359 };      // トラックバーの上限値
constexpr int    TRACK_N = sizeof(track_name) / sizeof(TCHAR*);

static_assert(TRACK_N == sizeof(track_default) / sizeof(int), "size of track_default mismatch with TRACK_N");
static_assert(TRACK_N == sizeof(track_s) / sizeof(int), "size of track_s mismatch with TRACK_N");
static_assert(TRACK_N == sizeof(track_e) / sizeof(int), "size of track_e mismatch with TRACK_N");


constexpr TCHAR *check_name[] = { "Select Object", "Analyze", "View Result", "Clear Result", "As English EXO?", "As Sub-filter/部分フィルター?", "Invert Position", "Save EXO", "Quick Blur", "Easy Privacy"}; // チェックボックスの名前
constexpr int   check_default[] = { -1, -1, 0, -1, 0, 0, 0, -1, 0, 0 }; // チェックボックスの初期値 (値は0か1)
constexpr int   CHECK_N = sizeof(check_name) / sizeof(TCHAR*);

static_assert(CHECK_N == sizeof(check_default) / sizeof(int), "size of check_default mismatch with CHECK_N");

#ifdef __AVX__
TCHAR* verstr={ "MotionTracking MK-II Plus AVX by Mr-Ojii\0" };
#else
TCHAR* verstr = { "MotionTracking MK-II Plus SSE2 by Mr-Ojii\0" };
#endif

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
	0, 0,							//	設定ウインドウのサイズ (FILTER_FLAG_WINDOW_SIZEが立っている時に有効)
	"MotionTracking MK-II Plus",	//	フィルタの名前
	TRACK_N,						//	トラックバーの数 (0なら名前初期値等もNULLでよい)
	const_cast<TCHAR **>(track_name),								//	トラックバーの名前郡へのポインタ
	const_cast<int *>(track_default),								//	トラックバーの初期値郡へのポインタ
	const_cast<int *>(track_s), const_cast<int *>(track_e),			//	トラックバーの数値の下限上限 (NULLなら全て0～256)
	CHECK_N,								//	チェックボックスの数 (0なら名前初期値等もNULLでよい)
	const_cast<TCHAR **>(check_name),		//	チェックボックスの名前郡へのポインタ
	const_cast<int *>(check_default),		//	チェックボックスの初期値郡へのポインタ
	func_proc,					//	フィルタ処理関数へのポインタ (NULLなら呼ばれません)
	NULL,						//	開始時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	NULL,						//	終了時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	func_update,						//	設定が変更されたときに呼ばれる関数へのポインタ (NULLなら呼ばれません)
	func_WndProc,				//	設定ウィンドウにウィンドウメッセージが来た時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	NULL, NULL,					//	システムで使いますので使用しないでください
	NULL,						//  拡張データ領域へのポインタ (FILTER_FLAG_EX_DATAが立っている時に有効)
	NULL,						//  拡張データサイズ (FILTER_FLAG_EX_DATAが立っている時に有効)
	verstr,
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

std::string modelDir;


//---------------------------------------------------------------------
//		フィルタ構造体のポインタを渡す関数
//---------------------------------------------------------------------
FILTER_DLL* filterlist[] = { &filter, &filter_hsv, &filter_bgs, NULL };
EXTERN_C FILTER_DLL __declspec(dllexport) ** __stdcall GetFilterTableList(void)
{
	char path[MAX_PATH * 2];
	if (GetModuleFileNameA(hModuleDLL, path, MAX_PATH * 2))
	{
		char* p = path;
		while (*p != '\0')
			p++;
		while (*p != '\\')
			p--;
		p++;
		*p = '\0';
		modelDir = std::string(path);
		modelDir += "MotionTracking_model\\";
	}

	return (FILTER_DLL**)&filterlist;
}

static cv::Scalar hue_to_scalar(int hue)
{
	hue = hue % 360;

	if (hue < 60)
		return cv::Scalar(255, hue * 255 / 60, 0);
	else if (hue < 120)
		return cv::Scalar((120-hue) * 255 / 60, 255, 0);
	else if (hue < 180)
		return cv::Scalar(0, 255, (hue - 120) * 255 / 60);
	else if (hue < 240)
		return cv::Scalar(0, (240 - hue) * 255 / 60, 255);
	else if (hue < 300)
		return cv::Scalar((hue - 240) * 255 / 60, 0, 255);
	else
		return cv::Scalar(255, 0, (360 - hue) * 255 / 60);
}

static void update_object_selection_window(int x1, int y1, int x2, int y2, FILTER* fp)
{
	//update only if visible
	if(!static_cast<bool>(cv::getWindowProperty("Object Selection", cv::WND_PROP_VISIBLE)))
		return;

	//draw the bounding box
	cv::Mat currentFrame;
	ocvImage.copyTo(currentFrame);
	cv::rectangle(currentFrame, cv::Point(x1, y1), cv::Point(x2, y2), hue_to_scalar(fp->track[1]), 2);

	imshow("Object Selection", currentFrame);
}

// Mouse callback function for object selection
static void onMouse(int event, int x, int y, int, void* fp_v)
{
	FILTER* fp = reinterpret_cast<FILTER*>(fp_v);
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
		boundingBox.x = std::clamp(static_cast<double>(x), 0.0, boundingBox.x);
		boundingBox.y = std::clamp(static_cast<double>(y), 0.0, boundingBox.y);
		selectObj = true;
		startSel = false;
		break;
	case cv::EVENT_MOUSEMOVE:

		if (startSel && !selectObj)
		{
			update_object_selection_window(boundingBox.x, boundingBox.y, x, y, fp);
		}
		break;
	}
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
BOOL func_update(FILTER *fp, int status)
{
	if (status == FILTER_UPDATE_STATUS_TRACK + 1)
	{
		if (selectObj)
			update_object_selection_window(boundingBox.x, boundingBox.y, boundingBox.x + boundingBox.width, boundingBox.y + boundingBox.height, fp);
		
		// View Result
		if (fp->track[2])
			return TRUE;
	}
	return FALSE;
}
BOOL func_WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, void *editp, FILTER *fp)
{
	if (!fp->exfunc->is_filter_active(fp) || !fp->exfunc->is_editing(editp))
		return FALSE;

	switch (message)
	{
	case WM_COMMAND:
	{
		switch (wparam)
		{
		case MID_FILTER_BUTTON: //Object selection
		{
			int srcw, srch;
			if (!fp->exfunc->get_select_frame(editp, &selA, &selB))
			{
				MessageBox(NULL, "Cannot get selected frame number", "AviUtl API Error", MB_OK);
				return FALSE;
			}
			if (!fp->exfunc->get_pixel_filtered(editp, selA, NULL, &srcw, &srch))
			{
				MessageBox(NULL, "Cannot get frame size", "AviUtl API Error", MB_OK);
				return FALSE;
			}
			const int step = ((srcw + 1) * 3) & ~3;

			std::unique_ptr<uint8_t[]> aubgr = std::make_unique<uint8_t[]>(step * srch);
			if (!fp->exfunc->get_pixel_filtered(editp, selA, aubgr.get(), NULL, NULL))
			{
				MessageBox(NULL, "Cannot get image", "AviUtl API Error", MB_OK);
				return FALSE;
			}

			cv::Mat cvBuffer(srch, srcw, CV_8UC3, aubgr.get(), step);
			cv::flip(cvBuffer, cvBuffer, 0);
			cvBuffer.copyTo(ocvImage);

			cv::namedWindow("Object Selection", cv::WINDOW_KEEPRATIO);
			cv::setMouseCallback("Object Selection", onMouse, fp);
			cv::resizeWindow("Object Selection", srcw, srch);
			cv::imshow("Object Selection", ocvImage);
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
			int srcw, srch;
			if (!fp->exfunc->get_frame_size(editp, &srcw, &srch))
			{
				MessageBox(NULL, "Cannot get original frame size", "AviUtl API Error", MB_OK);
				return FALSE;
			}
			int frmw, frmh;
			if (!fp->exfunc->get_pixel_filtered(editp, selA, NULL, &frmw, &frmh))
			{
				MessageBox(NULL, "Cannot get frame size", "AviUtl API Error", MB_OK);
				return FALSE;
			}

			if (srcw != frmw || srch != frmh)
			{
				int res = MessageBoxA(NULL,
					"EN\n"
					"Resizing has been detected.\n"
					"You may not be able to get normal results.\n"
					"It is recommended that you disable resizing.\n"
					"Do you want to continue the analyze?\n"
					"JA\n"
					"動画のリサイズが検出されました。\n"
					"正常な結果が得られない可能性があります。\n"
					"リサイズを無効化することをお勧めします。\n"
					"Analyzeを続行しますか？"
					, "MotionTracking MK-II Plus", MB_ICONWARNING | MB_YESNO);
				if (res == IDNO) {
					return FALSE;
				}
			}

			track_result.clear();
			track_found.clear();
			cv::Rect2i box = boundingBox;
			//Correct for out-of-bound box
			if (box.br().x > frmw)
			{
				box.width = frmw - box.x;
			}
			if (box.br().y > frmh)
			{
				box.height = frmh - box.y;
			}
			int64 start_time = cv::getTickCount();
			// Create Tracker
			cv::Ptr<cv::Tracker> tracker;
			try
			{
				switch (fp->track[0] - 1) {
				case 0:
					tracker = cv::TrackerMIL::create();
					break;
				case 1:
					tracker = cv::TrackerKCF::create();
					break;
				case 2:
					tracker = cv::TrackerCSRT::create();
					break;
				case 3:
				{
					auto params = cv::TrackerGOTURN::Params();
					params.modelBin = modelDir + "goturn.caffemodel";
					params.modelTxt = modelDir + "goturn.prototxt";
					tracker = cv::TrackerGOTURN::create(params);
					break;
				}
				case 4:
				{
					auto params = cv::TrackerDaSiamRPN::Params();
					params.model = modelDir +  "dasiamrpn_model.onnx";
					params.kernel_r1 = modelDir + "dasiamrpn_kernel_r1.onnx";
					params.kernel_cls1 = modelDir + "dasiamrpn_kernel_cls1.onnx";
					tracker = cv::TrackerDaSiamRPN::create(params);
					break;
				}
				case 5:
				{
					auto params = cv::TrackerNano::Params();
					params.backbone = modelDir + "nanotrack_backbone_sim.onnx";
					params.neckhead = modelDir + "nanotrack_head_sim.onnx";
					tracker = cv::TrackerNano::create(params);
					break;
				}
				default:
				{
					//なんか2つモデルがあるが、上のほうが良い？
					//https://github.com/opencv/opencv_extra/blob/4.x/testdata/dnn/onnx/models/vitTracker.onnx
					//https://github.com/opencv/opencv_zoo/blob/main/models/object_tracking_vittrack/object_tracking_vittrack_2023sep.onnx

					auto params = cv::TrackerVit::Params();
					params.net = modelDir + "vitTracker.onnx";
					tracker = cv::TrackerVit::create(params);
					break;
				}
				}
			}
			catch (cv::Exception e)
			{
				MessageBox(NULL, e.what(), "OpenCV3 Error", MB_OK);
				return FALSE;
			}
			catch (...)
			{
				//nullptr
				tracker = cv::Ptr<cv::Tracker>();
			}
			if (!tracker)
			{
				MessageBox(NULL, "Error when creating tracker", "OpenCV3 Error", MB_OK);
				return FALSE;
			}

			bool track_init = false;
			track_result.clear();
			track_found.clear();
			//Loop through selected frames for analysis
			TCHAR shortmsg[64] = { 0 };
			int64 prev_stamp, new_stamp;
			const int step = ((frmw + 1) * 3) & ~3;
			std::unique_ptr<uint8_t[]> nextau = std::make_unique<uint8_t[]>(step * frmh);
			for (int f = selA; f <= selB; f++)
			{
				//Set next frame
				if (!fp->exfunc->get_pixel_filtered(editp, f, nextau.get(), NULL, NULL))
				{
					MessageBox(NULL, "Cannot get next image", "AviUtl API Error", MB_OK);
					return FALSE;
				}

				cv::Mat cvNext(frmh, frmw, CV_8UC3, nextau.get(), step);
				cv::flip(cvNext, cvNext, 0);

				if (!track_init)
				{
					//TODO
					SecureZeroMemory(shortmsg, sizeof(TCHAR[64]));
					sprintf_s(shortmsg, "%s tracker initializing...", track_method[fp->track[0] - 1]);
					SetWindowText(fp->hwnd, shortmsg);
					try
					{
						tracker->init(cvNext, boundingBox);
					}
					catch (...)
					{
						MessageBox(NULL, "Error initializing tracker", "OpenCV3 Error", MB_OK);
						return FALSE;
					}
					track_init = true;
					track_found.push_back(true);

					prev_stamp = cv::getTickCount();
				}
				else
				{
					new_stamp = cv::getTickCount();
					double fps = 1.0 / ((new_stamp - prev_stamp) / cv::getTickFrequency());
					SecureZeroMemory(shortmsg, sizeof(TCHAR[64]));
					sprintf_s(shortmsg, "%s processing frame %d/%d @%.2f fps", track_method[fp->track[0] - 1], f + 1, selB + 1, fps);
					SetWindowText(fp->hwnd, shortmsg);
					prev_stamp = cv::getTickCount();

					try {
						if (tracker->update(cvNext, box))
						{
							track_found.push_back(true);
						}
						else
						{
							track_found.push_back(false);
						}
					}
					catch (...)
					{
						MessageBox(NULL, "Obscure tracker error", "Tracker update exception", MB_OK);
						return FALSE;
					}
				}
				if (box.area() <= 0)
				{
					box.width = 20;
					box.height = 20;
				}
				if (box.x < 0) {
					box.width += box.x;
					box.x = 0;
				}
				if (box.y < 0) {
					box.height += box.y;
					box.y = 0;
				}
				if (box.br().x > frmw)
				{
					box.width = frmw - box.x;
				}
				if (box.br().y > frmh)
				{
					box.height = frmh - box.y;
				}
				track_result.push_back(box);
			}
			int64 end_time = cv::getTickCount();
			double run_time = (end_time - start_time) / cv::getTickFrequency();
			SecureZeroMemory(shortmsg, sizeof(TCHAR[64]));
			sprintf_s(shortmsg, "%d frames tracked in %.2f sec", selB - selA, run_time);
			SetWindowText(fp->hwnd, shortmsg);
			SecureZeroMemory(shortmsg, sizeof(TCHAR[64]));
			sprintf_s(shortmsg, "Tracking Completed!\nAverage %.2f fps", (selB - selA) / run_time);
			MessageBox(NULL, shortmsg, "Tracking Completed!", MB_OK);
			return TRUE;

			break;
		}
		case MID_FILTER_BUTTON + 3: //Clear data
		{
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
		case MID_FILTER_BUTTON + 7: //Save EXO
		{
			//TODO
			if (track_result.size() <= 0)
			{
				MessageBox(fp->hwnd, "No track data to save!", "Operation Error", MB_OK);
				return FALSE;
			}

			CHAR filename[MAX_PATH] = { 0 };
			if (!fp->exfunc->dlg_get_save_name(filename, "ExEdit Object File(*.exo)\0*.exo;\0", "tracking.exo"))
				return TRUE;

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

						if (fp->check[6]) //Invert Position
						{
							Xi = -Xi;
							Xf = -Xf;
							Yi = -Yi;
							Yf = -Yf;
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
						if (fp->check[6]) // invert position
						{
							Xi = -Xi;
							Xf = -Xf;
							Yi = -Yi;
							Yf = -Yf;
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
	}
	
	return FALSE;
}

BOOL func_proc(FILTER *fp, FILTER_PROC_INFO *fpip)
{
	bool isFilterActive, isEditing, isFrameInRng, hasResult, redraw, isSaving;
	redraw = false;
	isFilterActive = (fp->exfunc->is_filter_active(fp) == TRUE);
	isEditing = (fp->exfunc->is_editing(fpip->editp) == TRUE);
	hasResult = track_result.size() > fpip->frame - selA;
	isFrameInRng = (fpip->frame >= selA) && (fpip->frame <= selB);
	isSaving = (fp->exfunc->is_saving(fpip->editp) == TRUE);

	if (isFilterActive && isEditing && fp->check[2] && hasResult && isFrameInRng && !isSaving)
	{
		int frmsize = fpip->w * fpip->h;
		std::unique_ptr<PIXEL[]> aubuf = std::make_unique<PIXEL[]>(frmsize);
		std::unique_ptr<PIXEL_YC[]> yc_src = std::make_unique<PIXEL_YC[]>(frmsize);
		byte* p1 = (byte*)fpip->ycp_edit;
		byte* p2 = (byte*)yc_src.get();
		size_t yc_linesize = sizeof(PIXEL_YC) * fpip->w;
		size_t yc_maxlinesize = sizeof(PIXEL_YC) * fpip->max_w;
		for (int line = 0; line < fpip->h; line++)
		{
			memcpy_s(p2, yc_linesize, p1, yc_linesize);
			p1 += yc_maxlinesize;
			p2 += yc_linesize;
		}
		fp->exfunc->yc2rgb(aubuf.get(), yc_src.get(), frmsize);
		cv::Mat disp(fpip->h, fpip->w, CV_8UC3, aubuf.get());

		if (track_found[fpip->frame - selA])
		{
			cv::rectangle(disp, track_result[fpip->frame - selA], hue_to_scalar(fp->track[1]), 2, 1);
			cv::putText(disp, "OK", cv::Point(0, 50), cv::FONT_HERSHEY_PLAIN, 2.0, cv::Scalar(0, 255, 0), 2);
		}
		else
		{
			cv::putText(disp, "ERROR", cv::Point(0, 50), cv::FONT_HERSHEY_PLAIN, 2.0, cv::Scalar(0, 0, 255), 2);
		}

		PIXEL* aubuf2 = (PIXEL*)disp.data;
		std::unique_ptr<PIXEL_YC[]> ycbuf = std::make_unique<PIXEL_YC[]>(frmsize);
		fp->exfunc->rgb2yc(ycbuf.get(), aubuf2, frmsize);
		size_t linesize = sizeof(PIXEL_YC) * fpip->w;
		size_t canvas_linesize = sizeof(PIXEL_YC) * fpip->max_w;
		byte* ptemp = (byte*)fpip->ycp_temp;
		byte* psrc = (byte*)ycbuf.get();
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
	}

	if (isFilterActive && isEditing && fp->check[8] && hasResult && isFrameInRng)
	{
		if (track_found[fpip->frame - selA])
		{
			size_t frmsize = fpip->w* fpip->h;
			std::unique_ptr<PIXEL[]> aubuf = std::make_unique<PIXEL[]>(frmsize);
			std::unique_ptr<PIXEL_YC[]> yc_src = std::make_unique<PIXEL_YC[]>(frmsize);
			byte* p1 = (byte*)fpip->ycp_edit;
			byte* p2 = (byte*)yc_src.get();
			size_t yc_linesize = sizeof(PIXEL_YC)*fpip->w;
			size_t yc_maxlinesize = sizeof(PIXEL_YC)*fpip->max_w;
			for (int line = 0; line < fpip->h; line++)
			{
				memcpy_s(p2, yc_linesize, p1, yc_linesize);
				p1 += yc_maxlinesize;
				p2 += yc_linesize;
			}
			fp->exfunc->yc2rgb(aubuf.get(), yc_src.get(), frmsize);

			cv::Mat ocvbuf(fpip->h, fpip->w, CV_8UC3, aubuf.get());
			//cv::flip(ocvbuf, ocvbuf, 0);
			cv::Mat blurArea(ocvbuf, track_result[fpip->frame - selA]);
			cv::blur(blurArea, blurArea, cv::Size(21, 21));
			PIXEL* aubuf2 = (PIXEL*)ocvbuf.data;
			std::unique_ptr<PIXEL_YC[]> ycbuf = std::make_unique<PIXEL_YC[]>(frmsize);
			fp->exfunc->rgb2yc(ycbuf.get(), aubuf2, frmsize);
			size_t linesize = sizeof(PIXEL_YC)*fpip->w;
			size_t canvas_linesize = sizeof(PIXEL_YC)*fpip->max_w;
			byte* ptemp = (byte*)fpip->ycp_temp;
			byte* psrc = (byte*)ycbuf.get();
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

			redraw = true;
		}
	}
	if (fp->check[9] && isFilterActive && isEditing)
	{
		//AviUtl -> OCV
		size_t frmsize = fpip->w* fpip->h;
		std::unique_ptr<PIXEL[]> aubuf = std::make_unique<PIXEL[]>(frmsize);
		std::unique_ptr<PIXEL_YC[]> yc_src = std::make_unique<PIXEL_YC[]>(frmsize);
		byte* p1 = (byte*)fpip->ycp_edit;
		byte* p2 = (byte*)yc_src.get();
		size_t yc_linesize = sizeof(PIXEL_YC)*fpip->w;
		size_t yc_maxlinesize = sizeof(PIXEL_YC)*fpip->max_w;
		for (int line = 0; line < fpip->h; line++)
		{
			memcpy_s(p2, yc_linesize, p1, yc_linesize);
			p1 += yc_maxlinesize;
			p2 += yc_linesize;
		}
		fp->exfunc->yc2rgb(aubuf.get(), yc_src.get(), frmsize);
		cv::Mat ocvbuf(fpip->h, fpip->w, CV_8UC3, aubuf.get());
		//
		//Convert to greyscale
		cv::Mat ocvGrey;
		cv::cvtColor(ocvbuf, ocvGrey, cv::COLOR_BGR2GRAY, 1);
		//Haar part
		//Check if XML exists in root or plugin folder
		cv::CascadeClassifier frontface = cv::CascadeClassifier::CascadeClassifier();
		cv::CascadeClassifier profileface = cv::CascadeClassifier::CascadeClassifier();
		frontface.load(modelDir + "haarcascade_frontalface_default.xml");
		profileface.load(modelDir + "haarcascade_profileface.xml");
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
				std::unique_ptr<PIXEL_YC[]> ycbuf = std::make_unique<PIXEL_YC[]>(frmsize);
				fp->exfunc->rgb2yc(ycbuf.get(), aubuf2, frmsize);
				size_t linesize = sizeof(PIXEL_YC)*fpip->w;
				size_t canvas_linesize = sizeof(PIXEL_YC)*fpip->max_w;
				byte* ptemp = (byte*)fpip->ycp_temp;
				byte* psrc = (byte*)ycbuf.get();
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
			}
			
			//Clean up
						
			redraw = true;
		}
	}
	return redraw;
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		hModuleDLL = hinstDLL;
		break;
	}
	return TRUE;
}

