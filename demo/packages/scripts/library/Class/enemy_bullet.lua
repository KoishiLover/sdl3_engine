local utility = require("packages.scripts.library.Utility")
LoadTexture("tex.bullet","packages/resources/Images/bullet.png")
utility.LoadImageGroup('img.bullet','tex.bullet',0,0,150,1200/8,1,8)

---@class enemy_bullet
local enemy_bullet = {}     -- __index 由 New(enemy_bullet, ...) 自动补上

--Object.New 收集的是小写回调名（init/update/render/kill/collision…），大写不会被收集
function enemy_bullet:init(x,y,id)
    self.x = x
    self.y = y
    self.img = "img.bullet"..id
    self.omega = 0.5
    self:SetLayer(100)
end

function enemy_bullet:render()
    DrawImage(self.img,self.x,self.y,self.rot,1/3)
end

return enemy_bullet
