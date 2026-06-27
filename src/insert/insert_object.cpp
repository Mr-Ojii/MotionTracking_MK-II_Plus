#include "insert_object.hpp"

std::string InsertObject::make_alias(const std::vector<FRMFIX>& fixedFrm, int vi_start, int vi_end) {
    std::string s;
    s += "[Object]\n";
    s += "frame=";
    for (int i = vi_start; i <= vi_end; i++) {
        s += std::to_string(fixedFrm[i].frame);
        if (i < vi_end) s += ",";
    }
    s += "\n";
    s += "[Object.0]\n";
    s += "effect.name=図形\n";
    s += "図形の種類=四角形\n";
    s += "サイズ=100\n";
    s += "縦横比=0.00\n";
    s += "ライン幅=4\n";
    s += "色=00ff00\n";
    s += "角を丸くする=0\n";
    s += "[Object.1]\n";
    s += "effect.name=リサイズ\n";

    // 拡大率=s1,s2,...,直線移動,0
    s += "拡大率=";
    for (int i = vi_start; i <= vi_end; i++) {
        s += std::to_string((int)fixedFrm[i].scale);
        s += ".000";
        if (i < vi_end) s += ",";
    }
    s += ",直線移動,0\n";

    s += "X=100.000\nY=100.000\n補間なし=0\nピクセル数でサイズ指定=0\n";
    s += "[Object.2]\n";
    s += "effect.name=標準描画\n";

    // X=x1,x2,...,直線移動,0
    s += "X=";
    for (int i = vi_start; i <= vi_end; i++) {
        s += std::to_string(fixedFrm[i].cx) + ".00";
        if (i < vi_end) s += ",";
    }
    s += ",直線移動,0\n";

    // Y=y1,y2,...,直線移動,0
    s += "Y=";
    for (int i = vi_start; i <= vi_end; i++) {
        s += std::to_string(fixedFrm[i].cy) + ".00";
        if (i < vi_end) s += ",";
    }
    s += ",直線移動,0\n";

    s += "Z=0.00\n";
    s += "拡大率=100.00\n";
    s += "縦横比=0.000\n";
    s += "透明度=0.00\n";
    s += "合成モード=通常\n";

    s += "[Object.2]\n";
    s += "effect.name=リサイズ\n";
    s += "拡大率=100.000\n"; // 仮
    s += "X=100.000\n";
    s += "Y=100.000\n";
    s += "補間なし=0\n";
    s += "ピクセル数でサイズ指定=0\n";
    return s;
}

bool InsertObject::Insert(
    const std::vector<cv::Rect2d>& results,
    const std::vector<bool>& found,
    int rangeStart,
    EDIT_HANDLE* edit)
{
    if (results.empty()) return false;

    auto rect_list = results;
    auto err_list  = found;

    std::vector<UINT32>   inter_list;
    std::vector<FRMFIX>   fixedFrm;
    std::vector<FRMGROUP> groups;

    find_inter_frame(err_list, inter_list);

    struct Param {
        std::vector<cv::Rect2d>* rect_list;
        std::vector<bool>*       err_list;
        std::vector<UINT32>*     inter_list;
        std::vector<FRMFIX>*     fixedFrm;
        std::vector<FRMGROUP>*   groups;
        int  rangeStart;
        bool ok;
    } p { &rect_list, &err_list, &inter_list, &fixedFrm, &groups, rangeStart, false };

    edit->call_edit_section_param(&p, [](void* v, EDIT_SECTION* edit) {
        auto* p = static_cast<Param*>(v);

        fix_frame(*p->rect_list, *p->err_list, *p->inter_list,
                  *p->fixedFrm, edit->info->width, edit->info->height, p->rangeStart);
        groupObject(*p->fixedFrm, *p->groups, p->rangeStart);

        int layer = edit->info->layer;
        for (const auto& g : *p->groups) {
            std::string alias = make_alias(*p->fixedFrm, g.vi_start, g.vi_end);
            OBJECT_HANDLE handle = edit->create_object_from_alias(
                alias.c_str(), layer, g.start, g.end - g.start + 1);
            if (!handle) { // insert 失敗時
                p->ok = false;
                return;
            }
        }
        p->ok = true;
    });

    return p.ok;
}

cv::Point InsertObject::getCenter(const cv::Rect2d& box) {
    return cv::Point(
        (int)((box.tl().x + box.br().x) / 2),
        (int)((box.tl().y + box.br().y) / 2)
    );
}

//Find single-frame error to be interpolate
//RETURN: a std::vector<UINT32> containing relevant index -> out_list
//RETURN: no. of inter-frame ->func return int
int InsertObject::find_inter_frame(std::vector<bool> &err_list, std::vector<UINT32> &out_list)
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

void InsertObject::fix_frame(std::vector<cv::Rect2d> &rect_list, std::vector<bool> &err_list, std::vector<UINT32> &inter_list, std::vector<FRMFIX> &out, int frm_w, int frm_h, int rangeStart)
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
        buf.scale = std::max(rect_list[i].width, rect_list[i].height);
        buf.frame = (int)i + rangeStart;
        buf.found = err_list[i];
        out.push_back(buf); //store to output vector
    }
}

//Group into objects
void InsertObject::groupObject(std::vector<FRMFIX> &fixedframes, std::vector<FRMGROUP> &out, int rangeStart)
{
    //TODO
    std::vector<int> startpos;
    std::vector<int> endpos;
    bool prevstate = false;
    for (size_t i = 0; i < fixedframes.size(); i++)
    {
        bool currentstate = fixedframes[i].found;
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
            buf.start = buf.vi_start + rangeStart;
            buf.end = buf.vi_end + rangeStart;
            out.push_back(buf);
        }
    }
}
