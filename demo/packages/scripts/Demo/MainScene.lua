LoadTexture("tex.bg","packages/resources/Images/background.png")
LoadImage('img.bg','tex.bg',0,0,2560,1440)
SetImageCenter('img.bg',2560/2 , 1440/2)
LoadTexture("tex.start","packages/resources/Images/start.png")
LoadImage('img.start','tex.start',0,0,400,200)
SetImageCenter('img.start',200 , 100)
LoadTexture("tex.exit","packages/resources/Images/exit.png")
LoadImage('img.exit','tex.exit',0,0,400,200)
SetImageCenter('img.exit',200 , 100)

local Scene = require("packages.scripts.library.Scene.Scene")
---@class MainScene:Scene
local MainScene = {}
function MainScene:Init(mgr)
    Scene.Init(self , "MainScene",mgr)
    self.bg_img = "img.bg"
    self.title_img = {"img.start" , "img.exit"}
    self.fc = {255,255,255,255}
    self.ufc = {127,127,127,255}
    self.index = 1
end

function MainScene:Update()
    Scene.Update(self)
    if IsKeyDown(KEY.UP) then
        self.index = math.max(self.index-1,1)
    elseif IsKeyDown(KEY.DOWN) then
        self.index = math.min(self.index+1,#self.title_img)
    end
    if IsKeyDown(KEY.Z) then
        if self.index == 1 then
            self.mgr:changeScene("GameScene")
        else
            GameExit()
        end
    end
end

function MainScene:Render()
    SetWorldID(-1)
    DrawImage(self.bg_img ,480,270,0,960/2560)
    for i = 1, #self.title_img do
        local img = self.title_img[i]
        if i == self.index then
            SetImageState(img,BlendMode.None,255,255,255,255)
        else
            SetImageState(img,BlendMode.None,127,127,127,255)
        end
        DrawImage(img,480,200+100*(i-1),0,0.5)
    end
end

function MainScene.Create(mgr)
    local instance = {}
    instance.Init = MainScene.Init
    instance.Update = MainScene.Update
    instance.Render = MainScene.Render
    instance.Enter = Scene.Enter
    instance.Leave = Scene.Leave
    instance:Init(mgr)
    return instance
end

return MainScene
