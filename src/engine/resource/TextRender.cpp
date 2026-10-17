//
// @author 帽子屋小姐
// @date 2026/10/14
//

#include "TextRender.hpp"
#include "../engine_log.hpp"

namespace engine {

    TextRender::~TextRender() {
        Destroy();
    }

    bool TextRender::Init(SDL_Renderer* renderer) {
        using log = EngineLogger;
        if (m_engine) {
            return true;    // 已经初始化过
        }
        if (!renderer) {
            log::err("[engine] TextRender::Init 传入的 renderer 为空");
            return false;
        }
        //文本引擎依赖 TTF_Init；如果外部已经初始化过，销毁时就不该由我们 TTF_Quit
        if (!TTF_WasInit()) {
            if (!TTF_Init()) {
                log::err("[engine] 调用函数 TTF_Init 时出错:{}",SDL_GetError());
                return false;
            }
            m_ttf_owner = true;
        }
        m_renderer = renderer;
        if (m_engine = TTF_CreateRendererTextEngine(renderer); !m_engine) {
            log::err("[engine] 创建 TTF 文本引擎时出错:{}",SDL_GetError());
            return false;
        }
        log::info("[engine]  SDL_ttf 文本引擎初始化完成 版本:{}",TTF_Version());
        return true;
    }

    void TextRender::Destroy() noexcept {
        for (auto& [name ,font] : m_fonts) {
            if (font) {
                TTF_CloseFont(font);
                EngineLogger::info("[engine] 字体资源 [{}] 销毁",name);
            }
        }
        m_fonts.clear();
        if (m_text) {
            TTF_DestroyText(m_text);
            m_text = nullptr;
        }
        if (m_engine) {
            TTF_DestroyRendererTextEngine(m_engine);
            m_engine = nullptr;
        }
        m_renderer = nullptr;
        if (m_ttf_owner) {
            TTF_Quit();
            m_ttf_owner = false;
        }
    }

    TTF_Font* TextRender::FindFont(std::string_view name) const noexcept {
        //StringHash/StringEqual 支持 string_view 异构查找，不必构造临时 string
        const auto it = m_fonts.find(name);
        return it == m_fonts.end() ? nullptr : it->second;
    }

    void TextRender::LoadTTF(std::string_view name ,std::string_view path ,float ptsize) {
        using log = EngineLogger;
        if (name.empty() || path.empty()) {
            log::warn("[engine] LoadTTF 的名字或路径为空，已忽略");
            return;
        }
        if (!m_engine) {
            log::err("[engine] 调用 LoadTTF 之前必须先 Init");
            return;
        }
        if (m_fonts.contains(name)) {
            return;         // 同名已加载，忽略
        }
        if (ptsize <= 0.0f) {
            log::warn("[engine] 字体 [{}] 的点大小必须为正数，已回退到 16",name);
            ptsize = 16.0f;
        }
        const std::string file(path);
        TTF_Font* font = TTF_OpenFont(file.c_str() ,ptsize);
        if (!font) {
            log::err("[engine] 打开字体文件失败 [{}]:{}",path,SDL_GetError());
            return;
        }
        m_fonts.emplace(std::string(name) ,font);
        log::info("[engine] 字体资源 [{}] 创建成功",name);
    }

    void TextRender::SetTextSize(float size ,std::string_view ttf) {
        using log = EngineLogger;
        TTF_Font* font = FindFont(ttf);
        if (!font) {
            log::warn("[engine] SetTextSize 失败，字体 [{}] 未加载",ttf);
            return;
        }
        if (size <= 0.0f) {
            log::warn("[engine] 字体 [{}] 的点大小必须为正数，已忽略",ttf);
            return;
        }
        if (!TTF_SetFontSize(font ,size)) {
            log::err("[engine] 设置字体 [{}] 的点大小时出错:{}",ttf,SDL_GetError());
            return;
        }
        //字号变了，复用的文本对象要重建才能反映新尺寸
        if (m_text) {
            TTF_DestroyText(m_text);
            m_text = nullptr;
        }
    }

    void TextRender::SetTextColor(Uint8 r ,Uint8 g ,Uint8 b ,Uint8 a) {
        m_color = SDL_Color{r ,g ,b ,a};
    }

    void TextRender::SetTextAlign(HAlign h ,VAlign v) {
        m_halign = h;
        m_valign = v;
    }

    void TextRender::SetTextAlign() {
        m_halign = HAlign::Center;
        m_valign = VAlign::Middle;
    }

    void TextRender::AddTextWorld(int id ,int left ,int right ,int top ,int bottom) noexcept {
        if (id < 0 || id >= static_cast<int>(m_worlds.size())) {
            return;
        }
        m_worlds[id] = World{static_cast<float>(left)  ,static_cast<float>(right),
                             static_cast<float>(top)   ,static_cast<float>(bottom)};
    }

    void TextRender::SetTextWorldID(int id) noexcept {
        if (id < -1 || id >= static_cast<int>(m_worlds.size())) {
            return;
        }
        m_world_id = id;
    }

    bool TextRender::MapWorldRect(int& l ,int& r ,int& t ,int& b) const noexcept {
        if (m_world_id < 0 || !m_renderer) {
            return false;
        }
        //逻辑尺寸直接问 SDL，免得和 Renderer 各存一份、改了 SetLogicalPresentSize 之后不同步
        int lw = 0 ,lh = 0;
        SDL_RendererLogicalPresentation mode = SDL_LOGICAL_PRESENTATION_DISABLED;
        if (!SDL_GetRenderLogicalPresentation(m_renderer ,&lw ,&lh ,&mode) || lw <= 0 || lh <= 0) {
            return false;
        }
        const World& w = m_worlds[m_world_id];
        const float wx = (w.left + w.right) * 0.5f;     //世界中心
        const float wy = (w.top + w.bottom) * 0.5f;
        //与 Renderer::DrawImage 的世界模式同一套换算：
        //  lx = (x - wx) + 逻辑宽/2
        //  ly = 逻辑高/2 - (y - wy)
        //矩形尺寸不参与换算（世界换算是纯平移 + y 翻转），只把中心点搬过去，矩形本身不会被翻转
        const int ow = r - l ,oh = b - t;
        const int sx = static_cast<int>((l + r) * 0.5f - wx + static_cast<float>(lw) * 0.5f);
        const int sy = static_cast<int>(static_cast<float>(lh) * 0.5f - ((t + b) * 0.5f - wy));
        l = sx - ow / 2;
        r = l + ow;
        t = sy - oh / 2;
        b = t + oh;
        return true;
    }

    bool TextRender::MeasureText(std::string_view ttf ,std::string_view text ,int& w ,int& h) const {
        w = 0;
        h = 0;
        TTF_Font* font = FindFont(ttf);
        if (!font) {
            return false;
        }
        return TTF_GetStringSize(font ,text.data() ,text.size() ,&w ,&h);
    }

    void TextRender::DrawText(std::string_view ttf ,std::string_view text ,int l ,int r ,int t ,int b) {
        using log = EngineLogger;
        if (!m_engine) {
            log::err("[engine] 调用 DrawText 之前必须先 Init");
            return;
        }
        TTF_Font* font = FindFont(ttf);
        if (!font) {
            log::warn("[engine] DrawText 失败，字体 [{}] 未加载",ttf);
            return;
        }
        if (text.empty()) {
            return;
        }

        //复用同一个 TTF_Text：只更新字体/内容/颜色，SDL_ttf 内部按需重建
        if (!m_text) {
            m_text = TTF_CreateText(m_engine ,font ,text.data() ,text.size());
            if (!m_text) {
                log::err("[engine] 创建文本对象时出错:{}",SDL_GetError());
                return;
            }
        } else if (!TTF_SetTextFont(m_text ,font) ||
                   !TTF_SetTextString(m_text ,text.data() ,text.size())) {
            log::err("[engine] 更新文本内容时出错:{}",SDL_GetError());
            return;
        }
        if (!TTF_SetTextColor(m_text ,m_color.r ,m_color.g ,m_color.b ,m_color.a)) {
            log::err("[engine] 设置文本颜色时出错:{}",SDL_GetError());
            return;
        }

        int w = 0 ,h = 0;
        if (!TTF_GetTextSize(m_text ,&w ,&h)) {
            log::err("[engine] 获取文本尺寸时出错:{}",SDL_GetError());
            return;
        }

        //世界模式：先把矩形从世界坐标换算到渲染器逻辑坐标
        MapWorldRect(l ,r ,t ,b);

        //在矩形内按对齐方式定位
        int x = l ,y = t;
        switch (m_halign) {
            case HAlign::Left:   x = l;                       break;
            case HAlign::Center: x = l + (r-l-w)/2;           break;
            case HAlign::Right:  x = r - w;                   break;
        }
        switch (m_valign) {
            case VAlign::Top:    y = t;                       break;
            case VAlign::Middle: y = t + (b-t-h)/2;           break;
            case VAlign::Bottom: y = b - h;                   break;
        }

        if (!TTF_DrawRendererText(m_text ,static_cast<float>(x) ,static_cast<float>(y))) {
            log::err("[engine] 绘制文本时出错:{}",SDL_GetError());
        }
    }

}
