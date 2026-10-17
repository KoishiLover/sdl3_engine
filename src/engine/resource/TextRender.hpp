//
// @author 帽子屋小姐
// @date 2026/10/14
// @desc 基于 SDL3_ttf 的文本渲染
//       · 字体（.ttf/.otf）按名字缓存，TTF_OpenFont 打开，可随时 TTF_SetFontSize 改字号
//       · 文本走 TTF_Text + TTF_CreateRendererTextEngine：直接画到 renderer，不产生中间 surface
//       · 坐标是「渲染器逻辑坐标」：左上角原点、y 向下，单位由 SetLogicalPresentSize 定的逻辑尺寸决定
//       · 支持文本世界坐标：AddTextWorld 注册区域、SetTextWorldID 切换。
//         世界模式下 DrawText 的矩形按「世界坐标」解释（y 轴向上），换算公式与 Renderer::DrawImage
//         的世界模式完全一致，所以文字能和图像对齐到同一个世界。
//

#ifndef HATTA_ENGINE_TEXTRENDER_HPP
#define HATTA_ENGINE_TEXTRENDER_HPP


#include <unordered_map>
#include <array>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include <string_view>
#include "../engine_utils.hpp"

namespace engine {

    class TextRender {
    public:
        ///@brief 水平对齐（相对 DrawText 给的矩形）
        enum class HAlign : Uint8 { Left, Center, Right };
        ///@brief 垂直对齐（相对 DrawText 给的矩形）
        enum class VAlign : Uint8 { Top, Middle, Bottom };

        TextRender() = default;
        ~TextRender();

        TextRender(const TextRender&) = delete;
        TextRender& operator=(const TextRender&) = delete;

        ///@brief 初始化（TTF_Init + 创建文本引擎）；可重复调用，重复调用直接返回 true
        bool Init(SDL_Renderer* renderer);
        ///@brief 释放全部字体、文本对象与文本引擎；可重复调用
        void Destroy() noexcept;

        ///@brief 加载字体文件
        ///@param name 脚本里使用的名字
        ///@param path 字体文件路径（.ttf / .otf）
        ///@param ptsize 初始点大小；SDL_ttf 3.x 是 float，之后可任意缩放
        void LoadTTF(std::string_view name ,std::string_view path ,float ptsize = 16.0f);

        ///@brief 在矩形 [l,r]×[t,b] 内按当前对齐方式绘制文字
        ///@note 普通模式：逻辑坐标，左上原点、y 向下，要求 l<=r、t<=b
        ///      世界模式：世界坐标，y 轴向上；矩形按中心点换算（世界换算是纯平移，不会翻转矩形）
        void DrawText(std::string_view ttf ,std::string_view text,int l,int r,int t,int b);

        ///@brief 设置某个字体的点大小（改完立即生效，缓存的文本对象会被重建）
        void SetTextSize(float size,std::string_view ttf);
        ///@brief 设置后续 DrawText 使用的颜色
        void SetTextColor(Uint8 r,Uint8 g,Uint8 b,Uint8 a);
        ///@brief 设置文字在 DrawText 矩形内的对齐方式
        void SetTextAlign(HAlign h ,VAlign v);
        ///@brief 恢复默认对齐（水平居中 + 垂直居中）
        void SetTextAlign();

        ///@brief 注册/设置一个文本世界区域的边界：y 轴向上，left<=right、bottom<=top
        ///@note id 需在 [0, 8) 内，越界忽略；和 Renderer::AddWorld 是两套独立的世界表
        void AddTextWorld(int id ,int left ,int right ,int top ,int bottom) noexcept;
        ///@brief 切换当前文本世界：-1 表示普通坐标（左上原点、y 向下），其余取值需在 [0, 8) 内
        void SetTextWorldID(int id) noexcept;
        ///@brief 当前文本世界 id（-1 表示普通坐标）
        [[nodiscard]] int GetTextWorldID() const noexcept { return m_world_id; }

        ///@brief 量一段文字的像素尺寸（不创建文本对象）
        ///@return 字体未加载时返回 false，并把 w/h 置 0
        [[nodiscard]] bool MeasureText(std::string_view ttf ,std::string_view text ,int& w ,int& h) const;

    private:
        ///@brief 按名字取字体；未加载返回 nullptr
        [[nodiscard]] TTF_Font* FindFont(std::string_view name) const noexcept;

        ///@brief 当前处于世界模式时，把世界坐标矩形换算成渲染器逻辑坐标矩形
        ///@return 是否发生了换算（false = 普通模式或换算条件不满足，矩形保持原样）
        bool MapWorldRect(int& l ,int& r ,int& t ,int& b) const noexcept;

        std::unordered_map<std::string,TTF_Font*,StringHash,StringEqual> m_fonts;
        SDL_Renderer*   m_renderer = nullptr;
        TTF_TextEngine* m_engine   = nullptr;
        ///@brief 复用的文本对象，避免每帧创建/销毁
        TTF_Text*       m_text     = nullptr;
        SDL_Color       m_color{255 ,255 ,255 ,255};
        HAlign          m_halign = HAlign::Center;
        VAlign          m_valign = VAlign::Middle;
        ///@brief 文本世界表（与 Renderer 的世界表相互独立）
        std::array<World ,8> m_worlds;
        int             m_world_id = -1;
        ///@brief TTF_Init 是否由本类调用（决定 Destroy 时要不要 TTF_Quit）
        bool            m_ttf_owner = false;
    };

}

#endif //HATTA_ENGINE_TEXTRENDER_HPP
