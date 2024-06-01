// cpp_story_painter.cpp: 定义应用程序的入口点。
//
#include "main.h" // std


#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Grid.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Tooltip.H>

#include <minidocx.hpp>
#include <nlohmann/json.hpp>
#include <randomcolor.h>

const auto EM = 16;
const auto SPACE = EM / 2;
const auto WIDTH = 100 * 8;
const auto HEIGHT = 100 * 4;

using json = nlohmann::json;

RandomColor color_gen;
std::map<std::string, std::string> color_map;
static auto random_color(std::string id) {
    if (!color_map.contains(id)) {
        std::stringstream hex;
        hex << std::hex << std::setw(6) << std::setfill('0')
            << color_gen.generate(RandomColor::RandomHue, RandomColor::Dark);
        color_map[id] = hex.str();
    }
    return color_map[id];
}

auto cq_regex = std::regex("\\[CQ:.+?\\]");
auto space_regex = std::regex("^\\s");

class App : public Fl_Window {
private:
    json log = nullptr;
    static void pick_json(Fl_Widget*, void* _app) {
        auto app = (App*)_app;
        Fl_Native_File_Chooser native;
        native.title("选择日志文件");
        native.type(Fl_Native_File_Chooser::BROWSE_FILE);
        native.filter("*.json");
        switch (native.show()) {
        case -1:
            std::cout << native.errmsg() << std::endl;
            break;
        case 0:
            std::cout << native.filename() << std::endl;
            std::ifstream f(native.filename());
            app->log = json::parse(f);
            f.close();
            break;
        }
    }


    static void export_docx(Fl_Widget*, void* _app) {
        auto app = (App*)_app;
        if (app->log == nullptr) {
            fl_message("请先导入 JSON 文件");
            return;
        }
        docx::Document doc;
        for (auto& item : app->log["items"]) {
            auto msg = item["message"].get<std::string>();
            msg = std::regex_replace(msg, cq_regex, "");
            msg = std::regex_replace(msg, space_regex, "");
            std::cout << msg << std::endl;
            if (app->commandHide && (
                msg.starts_with(".") || msg.starts_with("。")
                )) {
                std::cout << "跳过骰子指令：" << msg << std::endl;
                continue;
            }
            if (app->offTopicHide && (
                msg.starts_with("(") || msg.starts_with("（")
                )) {
                std::cout << "跳过场外发言：" << msg << std::endl;
                continue;
            }
            if (msg.empty()) continue;
            auto p = doc.AppendParagraph();
            auto r = p.AppendRun();
            auto nn = item["nickname"].get<std::string>();
            auto id = item["IMUserId"].get<std::string>();
            r.AppendText(nn);
            r.AppendText("  ");
            if (!app->userIdHide) {
                r.AppendText("(");
                r.AppendText(id);
                r.AppendText(")");
            }
            if (!app->timeHide) {
                auto t = item["time"].get<std::time_t>();
                std::tm *time = std::localtime(&t);
                std::stringstream ss;
                if (app->yearHide) ss << std::put_time(time, "%H:%M:%S");
                else  ss << std::put_time(time, "%Y/%m/%e %H:%M:%S");
                r.AppendText(ss.str());
            }
            auto color = random_color(id);
            r.SetFontColor(color);
            if (app->msgPara) {
                auto msg_p = doc.AppendParagraph();
                auto msg_r = msg_p.AppendRun();
                msg_r.AppendText(msg);
                msg_r.SetFontColor(color);
                if (app->msgIndent) {
                    msg_p.SetLeftIndentChars(2);
                }
            }
            else {
                r.AppendLineBreak();
                if (app->msgIndent) r.AppendTabs();
                r.AppendText(msg);
            }
        }
        doc.Save("out.docx");
        auto p = std::string("file:///") + std::filesystem::current_path().string();
        fl_open_uri(p.c_str());
    }

    Fl_Grid* opt_view = nullptr;

    auto add_opt_toggle(int row, int col, const char *s, const bool *v) {
        auto b = new Fl_Check_Button(0, 0, 0, 0, s);
        b->value(*v);
        b->callback([](auto w, auto *d) {
            auto btn = (Fl_Check_Button *) w;
            auto b = (bool *) d;
            *b = btn->value();
        }, (void *) v);
        opt_view->widget(b, row, col);
        return b;
    }

public:
    bool commandHide = false;
    bool imageHide = true;
    bool offTopicHide = false;
    bool timeHide = false;
    bool userIdHide = true;
    bool yearHide = true;
    bool msgPara = false;
    bool msgIndent = false;
    App(int argc, char **argv) : Fl_Window(WIDTH, HEIGHT) {
        auto that = this;
        this->label("StoryPainter / 跑团日志染色器");
        const auto OPT_W = 300;
        opt_view = new Fl_Grid(SPACE, SPACE,
                               OPT_W  - SPACE * 2,
                               HEIGHT - SPACE * 2);
        opt_view->box(FL_SHADOW_BOX);
        opt_view->layout(9, 2, SPACE, SPACE);
        add_opt_toggle(0, 0, "骰子指令过滤", &commandHide)
            ->tooltip("开启后，不显示pc指令，正常显示指令结果");
        add_opt_toggle(0, 1, "表情图片过滤", &imageHide)
            ->tooltip("因为没有实现插入图片，实际上这个选项是摆设");
        add_opt_toggle(1, 0, "场外发言过滤", &offTopicHide)
            ->tooltip("开启后，所有以(和（为开头的发言将被豹豹吃掉不显示");
        add_opt_toggle(1, 1, "时间显示过滤", &timeHide)
            ->tooltip("开启后，日期和时间会被豹豹丢入海里不显示");
        add_opt_toggle(2, 0, "平台帐号隐藏", &userIdHide)
            ->tooltip("开启后，IM 平台账号（如 QQ 号）将在导出结果中不显示");
        add_opt_toggle(2, 1, "年月日不展示", &yearHide)
            ->tooltip("开启后，导出结果的日期将只显示几点几分");
        add_opt_toggle(3, 0, "消息独立成段", &msgPara)
            ->tooltip("如果你希望昵称和消息之间存在段间距");
        add_opt_toggle(3, 1, "消息段左缩进", &msgIndent)
            ->tooltip("缩进 2 字符或者添加一个制表符可能更加美观");
        auto pick_json_btn = new Fl_Button(0, 0, 0, 0, "导入 JSON");
        pick_json_btn->box(FL_SHADOW_BOX);
        pick_json_btn->callback(pick_json,that);
        opt_view->widget(pick_json_btn, 5, 0);
        auto export_docx_btn = new Fl_Button(0, 0, 0, 0, "导出 DOCX");
        export_docx_btn->box(FL_SHADOW_BOX);
        export_docx_btn->callback(export_docx, that);
        opt_view->widget(export_docx_btn, 5, 1);
        opt_view->end();
        this->size_range(OPT_W, HEIGHT);
        this->end();
        this->show(argc, argv);
    }
};



int main(int argc, char **argv) {
#if WIN32
    system("chcp 65001 > nul");
#endif
    Fl::get_system_colors();
    Fl::set_boxtype(FL_UP_BOX, FL_SHADOW_BOX);
    Fl::visible_focus(0);
    Fl_Tooltip::delay(0.1);
    auto app = App(argc, argv);
    return Fl::run();
}
