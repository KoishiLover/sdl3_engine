require("packages.scripts.library.Api")
require("packages.scripts.library.key")
local MainScene = require("packages.scripts.Demo.MainScene")
local GameScene = require("packages.scripts.Demo.GameScene")
local SceneManager = require("packages.scripts.library.Scene.SceneManager")

--窗口标题（不含 FPS）；同时用它验证文本渲染
local TITLE = "和吕建强的伪娘百合被骚福瑞百破后双双恶堕成母猪肉便器"

global_scene_manager = {}

--==== 帧率波动探针 ====
--抓住"明显波动"的瞬间，把当时的 GameObject 个数记进日志，
--用来定位"对象数量涨到多少开始掉帧"。
local FPS_DROP     = 5.0    --相对基准掉多少帧算明显波动（按需调）
local FPS_RECOVER  = 0.02   --基准回落的快慢：越低越"记仇"
local fps_base     = 0      --基准帧率：涨了立刻跟，跌了只缓慢跟
local fps_last     = 0      --上一次记录的帧率
local fps_in_drop  = false  --是否已处于波动中（只在状态翻转时记录，避免刷屏）

local function ProbeFPS()
    local fps = GetFPS()
    if fps <= 0 then
        return
    end
    if fps_base <= 0 then               --第一帧：建立基准
        fps_base = fps
        fps_last = fps
        return
    end

    --基准：涨了立刻跟随；跌了只缓慢下滑，免得基准被掉帧一路拖下去
    if fps > fps_base then
        fps_base = fps
    else
        fps_base = fps_base - (fps_base - fps) * FPS_RECOVER
    end

    --① 低于基准一定幅度  或者  ② 单帧比上一次掉了不少  →  算明显波动
    local dropping = (fps < fps_base - FPS_DROP) or (fps_last - fps > FPS_DROP)

    if dropping and not fps_in_drop then
        LogWarn(string.format(
            "帧率波动: %.2f 帧 (基准 %.2f, 上一帧 %.2f, 掉 %.2f)  GameObject = %d",
            fps, fps_base, fps_last, fps_base - fps, Object.Count()))
    elseif (not dropping) and fps_in_drop then
        LogInfo(string.format(
            "帧率恢复: %.2f 帧 (基准 %.2f)  GameObject = %d",
            fps, fps_base, Object.Count()))
    end

    fps_in_drop = dropping
    fps_last = fps
end

function GameInit()
    SetResolution(1920,1080)
    SetLogicalPresentSize(960,540)
    SetFPS(60)

    LoadTTF("font.title","packages/resources/Font/jnbbh-Bold.ttf",24)

    global_scene_manager = SceneManager.Create()
    local main_scene = MainScene.Create(global_scene_manager)
    local game_scene = GameScene.Create(global_scene_manager)
    global_scene_manager:addScene(main_scene.name , main_scene)
    global_scene_manager:addScene(game_scene.name,game_scene)
    global_scene_manager:changeScene(main_scene.name)
end

function GameUpdate()
    ProbeFPS()

    SetTitle(TITLE..string.format("   fps:%.2f",GetFPS()).."   object:"..Object.Count())

    global_scene_manager:Update()
end

function GameRender()
    global_scene_manager:Render()


    --SetTextWorldID(-1)
    --SetTextColor(60,40,70,255)
    --SetTextAlign(HAlign.Center,VAlign.Middle)
    --DrawText("font.title",TITLE,0,960,0,64)
end
