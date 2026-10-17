local utility = require('packages.scripts.library.Utility')
LoadTexture("tex.player","packages/resources/Images/player.png")
utility.LoadImageGroup("img.player","tex.player",0,0,300/8,169/3,8,3)

---@class player
local player = {}               -- __index 由 New(player) 自动补上，无需手写

function player:init()
    self.x = 0
    self.y = -120
    self.img = "img.player"
    self:SetLayer(200)          -- layer / group 要同步管理器的渲染集 / 碰撞组链表，必须走方法
    self:SetGroup(1)
    self.v = 3
    self.lr_flag = 0
    self.ud_flag = 0
    self.fire_flag = 0

    self.lr = 1
    self.l0 = 9
    self.r0 = 17
    self.imgid = 1
end

function player:updateFlag()
    self.lr_flag = 0
    self.ud_flag = 0
    if IsKeyDown(KEY.LEFT) then
        self.lr_flag = -1
    elseif IsKeyDown(KEY.RIGHT) then
        self.lr_flag = 1
    end
    if IsKeyDown(KEY.UP) then
        self.ud_flag = 1
    elseif IsKeyDown(KEY.DOWN) then
        self.ud_flag = -1
    end
end

function player:updateWalkImage()
    if self.lr >=1 and self.lr <= 8 then
        if self.lr_flag == 0 then
            self.imgid = math.floor(self.ani_timer/8)%8+1
        elseif self.lr_flag == -1 then
            self.lr = 9
        else
            self.lr = 17
        end
    elseif self.lr >= 9 and self.lr <= 16 then
        if self.lr_flag ~= -1 then
            self.lr = self.lr-1
            self.imgid = self.lr
            if self.lr == 9 then
                if self.lr_flag == 0 then
                    self.lr = 1
                else
                    self.lr = 17
                end
            end
        else
            if self.lr < 16 then
                self.lr = self.lr+1
                self.imgid = self.lr
            else
                self.imgid = 10+math.floor(self.ani_timer/8)%7
            end
        end
    elseif self.lr >= 17 and self.lr <= 24 then
        if self.lr_flag ~= 1 then
            self.lr = self.lr-1
            self.imgid = self.lr
            if self.lr == 17 then
                if self.lr_flag == 0 then
                    self.lr = 1
                else
                    self.lr = 9
                end
            end
        else
            if self.lr < 24 then
                self.lr = self.lr+1
                self.imgid = self.lr
            else
                self.imgid = 18+math.floor(self.ani_timer/8)%7
            end
        end
    end
end

function player:updateSpeed()
    local vx = self.lr_flag * self.v
    local vy = self.ud_flag * self.v
    local len = math.sqrt(vx*vx + vy*vy)
    if len > 0 then                 -- 没方向输入时 length 为 0，直接除以它会得到 nan
        vx = vx / len * self.v
        vy = vy / len * self.v
    end
    self.vx ,self.vy = vx ,vy
    if self.x <= -200+37.5/2 then
        self.x = -200+37.5/2
    elseif self.x>=200-37.5/2 then
        self.x = 200-37.5/2
    end
    local h =169/3
    if self.y >=225-h/2 then
        self.y = 225-h/2
    elseif self.y<= -225+h/2 then
        self.y = -225+h/2
    end
end

function player:update()
    self:updateFlag()
    self:updateSpeed()
    self:updateWalkImage()
end

function player:render()
    DrawImage(self.img..self.imgid,self.x,self.y,0,1.0)
end

return player
