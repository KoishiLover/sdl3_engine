LoadTexture('tex.uibg','packages/resources/Images/uibg.png')
LoadImage('img.uibg','tex.uibg',0,0,2560,1440)
SetImageCenter('img.uibg',1280,720)
local Scene = require("packages.scripts.library.Scene.Scene")
local player = require("packages.scripts.library.Class.player")
local enemy_bullet = require("packages.scripts.library.Class.enemy_bullet")
---@class GameScene:Scene
local GameScene = {}
function GameScene:Init(mgr)
    Scene.Init(self,'GameScene',mgr)

    AddWorld(1,-200,200,250,-250)
    New(player)
    self.tasks:add(function()
        local i = 0
        for x = -200,200 ,1 do
            for y = 255,-225,-5 do
                New(enemy_bullet ,x,y,i+1)
                i = (i+1)%8
                self.tasks:wait(1)
            end
        end
    end)

end

function GameScene:Update()
    Scene.Update(self)
    Object.Update()
    Object.UpdateXY()
    Object.UpdateLast()
end

function GameScene:Render()
    SetWorldID(1)
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