--[[
    将 hatta 命名空间下的所有 API 导入到全局，
]]

local Class = require("packages.scripts.library.Class")

-- 框架
SetFPS         = hatta.SetFPS
SetTitle       = hatta.SetTitle
SetVsync       = hatta.SetVsync
SetResolution  = hatta.SetResolution
SetWindowed    = hatta.SetWindowed
GetFPS         = hatta.GetFPS
GameExit       = hatta.GameExit

-- 渲染
BlendMode             = hatta.BlendMode
LoadTexture           = hatta.LoadTexture
LoadImage             = hatta.LoadImage
DrawImage             = hatta.DrawImage
SetImageState         = hatta.SetImageState
SetImageScale         = hatta.SetImageScale
SetImageCenter        = hatta.SetImageCenter
SetLogicalPresentSize = hatta.SetLogicalPresentSize
AddWorld              = hatta.AddWorld
SetWorldID            = hatta.SetWorldID

-- 音频
LoadMusic      = hatta.LoadMusic
PlayMusic      = hatta.PlayMusic
StopMusic      = hatta.StopMusic
PauseMusic     = hatta.PauseMusic
ResumeMusic    = hatta.ResumeMusic
SetMusicVolume = hatta.SetMusicVolume
LoadSE         = hatta.LoadSE
PlaySE         = hatta.PlaySE
SetSEVolume    = hatta.SetSEVolume

-- 键盘输入
GetKeyState = hatta.GetKeyState
IsKeyDown   = hatta.IsKeyDown

-- 日志（写 userdata/log/engine.log）
--   LogInfo / LogWarn 只记录；LogErr 是致命的：弹消息框并退出进程
LogInfo = hatta.LogInfo
LogWarn = hatta.LogWarn
LogErr  = hatta.LogErr

-- 文本
--   LoadTTF(name, path, ptsize) → DrawText(字体名, 文本, l, r, t, b)
--   默认按矩形居中；SetTextWorldID(-1) 是屏幕坐标（左上原点、y 向下），
--   切到文本世界后矩形按世界坐标解释（y 向上），世界表与 AddWorld 相互独立
HAlign         = hatta.HAlign
VAlign         = hatta.VAlign
LoadTTF        = hatta.LoadTTF
DrawText       = hatta.DrawText
SetTextSize    = hatta.SetTextSize
SetTextColor   = hatta.SetTextColor
SetTextAlign   = hatta.SetTextAlign
AddTextWorld   = hatta.AddTextWorld
SetTextWorldID = hatta.SetTextWorldID

-- 对象系统
--   创建对象用 New(MyClass, ...)：等价 Object.New，但会自动补上 MyClass.__index = MyClass
--   Object 另提供：Count / Capacity / Clear / SetBound / IsInBound / Pause / IsPaused / Collision
--   帧驱动（引擎不自动调用，调用时机由你自己决定）：
--     BeforeUpdate / Update / AfterUpdate / UpdateXY / UpdateLast / BoundCheck / Render
New        = Class.New
Object     = hatta.Object
GameObject = hatta.GameObject
