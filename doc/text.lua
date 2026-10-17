---@meta
---@author 帽子屋小姐
---文本渲染（基于 SDL3_ttf）
---@see hatta.LoadTTF
---@see hatta.DrawText

--[[
    用法示例：

        -- 1. 加载字体（.ttf / .otf）
        LoadTTF("font.main", "packages/resources/Font/jnbbh-Bold.ttf", 24)

        -- 2. 屏幕坐标下画一行字（顶部横条里居中）
        SetTextWorldID(-1)                              -- -1 = 屏幕坐标
        SetTextColor(60, 40, 70, 255)
        SetTextAlign(HAlign.Center, VAlign.Middle)
        DrawText("font.main", "HELLO", 0, 960, 0, 64)

        -- 3. 世界坐标下画字（与 DrawImage 的世界模式同一套换算）
        AddTextWorld(1, -200, 200, 250, -250)           -- 文本自己的世界表
        SetTextWorldID(1)
        DrawText("font.main", "LABEL", -100, 100, -100, -60)
        SetTextWorldID(-1)

    要点：
      · 文本坐标系和 SetLogicalPresentSize 的逻辑尺寸一致（本作为 960×540），不用手动乘缩放
      · 文本世界表与 AddWorld 的世界表**相互独立**，可以一个在屏幕坐标、一个在世界坐标
      · DrawText 的矩形只用来定位（配合对齐方式），**不会裁剪**
]]

---@class hatta.HAlign 水平对齐（相对 DrawText 给的矩形）
---@field Left integer 左对齐
---@field Center integer 水平居中（默认）
---@field Right integer 右对齐
hatta.HAlign = {}

---@class hatta.VAlign 垂直对齐（相对 DrawText 给的矩形）
---@field Top integer 顶对齐
---@field Middle integer 垂直居中（默认）
---@field Bottom integer 底对齐
hatta.VAlign = {}


---加载字体文件
---@param name string 脚本里使用的字体名
---@param path string 字体文件路径（.ttf / .otf）
---@param ptsize number 初始点大小，可带小数；之后可用 SetTextSize 改
---@note 同名重复加载会被忽略
---@note 文件打不开走 EngineLogger::err（弹框并退出进程）
function hatta.LoadTTF(name, path, ptsize) end


---在矩形 [l,r]×[t,b] 内按当前对齐方式绘制文字
---@param ttf string 已加载的字体名
---@param text string 要绘制的文本
---@param l number 矩形左边界
---@param r number 矩形右边界
---@param t number 矩形上边界
---@param b number 矩形下边界
---@note 屏幕模式（SetTextWorldID(-1)）：逻辑坐标，左上原点、y 向下，要求 l<=r、t<=b
---@note 世界模式：世界坐标，y 轴向上；矩形按**中心点**换算，所以照常传 l<r、t<b 即可，矩形不会被翻转
---@note 颜色由 SetTextColor 决定，对齐由 SetTextAlign 决定
function hatta.DrawText(ttf, text, l, r, t, b) end


---设置某个字体的点大小（下一个 DrawText 立即生效）
---@param size number 新的点大小，必须为正数
---@param ttf string 字体名
function hatta.SetTextSize(size, ttf) end


---设置后续 DrawText 使用的颜色
---@param r integer 0-255
---@param g integer 0-255
---@param b integer 0-255
---@param a integer 0-255
function hatta.SetTextColor(r, g, b, a) end


---设置文字在 DrawText 矩形内的对齐方式
---@param h integer HAlign.Left / HAlign.Center / HAlign.Right
---@param v integer VAlign.Top / VAlign.Middle / VAlign.Bottom
function hatta.SetTextAlign(h, v) end


---注册/设置一个文本世界区域的边界
---@param id integer 世界编号，取值 [0,8)，越界忽略
---@param left number 左边界
---@param right number 右边界
---@param top number 上边界（y 轴向上，需 >= bottom）
---@param bottom number 下边界
---@note 与 AddWorld 是两套独立的世界表，互不影响
function hatta.AddTextWorld(id, left, right, top, bottom) end


---切换当前文本世界
---@param id integer -1 表示屏幕坐标（左上原点、y 向下）；其余取值需在 [0,8) 内，越界忽略
function hatta.SetTextWorldID(id) end
