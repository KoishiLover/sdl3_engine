require("packages.scripts.library.Api")
require("packages.scripts.library.key")
local MainScene = require("packages.scripts.Demo.MainScene")
local GameScene = require("packages.scripts.Demo.GameScene")
local SceneManager = require("packages.scripts.library.Scene.SceneManager")

global_scene_manager = {}

function GameInit()
    SetResolution(1920,1080)
    SetLogicalPresentSize(960,540)
    SetFPS(60)

    global_scene_manager = SceneManager.Create()
    local main_scene = MainScene.Create(global_scene_manager)
    local game_scene = GameScene.Create(global_scene_manager)
    global_scene_manager:addScene(main_scene.name , main_scene)
    global_scene_manager:addScene(game_scene.name,game_scene)
    global_scene_manager:changeScene(main_scene.name)
end

function GameUpdate()
    SetTitle("和吕建强的伪娘百合被骚福瑞百破后双双恶堕成母猪肉便器"..string.format("   fps:%.2f",GetFPS()))
    global_scene_manager:Update()
end

function GameRender()
    global_scene_manager:Render()
end
