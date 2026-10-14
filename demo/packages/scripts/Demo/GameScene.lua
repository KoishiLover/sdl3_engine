LoadTexture('tex.uibg','packages/resources/Images/uibg.png')
LoadImage('img.uibg','tex.uibg',0,0,2560,1440)
SetImageCenter('img.uibg',1280,720)
local Scene = require("packages.scripts.library.Scene.Scene")
local player = require("packages.scripts.library.Class.player")
---@class GameScene:Scene
local GameScene = {}
function GameScene:Init(mgr)
    Scene.Init(self,'GameScene',mgr)
    --AddWorld(1,280,680,520,20)
    AddWorld(1,-200,200,250,-250)
    New(player)     -- 等价 Object.New，但会自动补上 player.__index = player
end

function GameScene:Update()
    Scene.Update(self)
    Object.Update()
    Object.UpdateXY()
    Object.UpdateLast()
end

function GameScene:Render()
    SetWorldID(1)   -- 本场景用世界 1 坐标；渲染前设置，避免影响其它场景
    Object.Render()
    DrawImage("img.uibg",0,0,0,960/2560)
    SetWorldID(-1)
end

function GameScene.Create(mgr)
    local instance = {}
    instance.Init = GameScene.Init
    instance.Update = GameScene.Update
    instance.Render = GameScene.Render
    instance.Enter = Scene.Enter
    instance.Leave = Scene.Leave
    instance:Init(mgr)
    return instance
end

return GameScene