local Scene = require("packages.scripts.library.Scene.Scene")
---@class SceneManager
local SceneManager = {}

function SceneManager:Init()
    ---@type table<string,Scene>
    self.scenes = {}
    self.current_scene = nil
    self.is_menu = true
end

function SceneManager:Update()
    self.current_scene:Update()
end

function SceneManager:Render()
    self.current_scene:Render()
end

function SceneManager:addScene(name , scene)
    if self.scenes[name] == nil then
        self.scenes[name] = scene
    end
end

function SceneManager:getScene(name)
    return self.scenes[name]
end

function SceneManager:changeScene(name)
    local cur = self.current_scene
    local next = self:getScene(name)
    if cur and cur.Leave then
        cur:Leave()
    end
    if next and next.Enter then
        next:Enter()
    end
    self.current_scene = self:getScene(name)
end

function SceneManager.Create()
    local manager = {}
    for k,v in pairs(SceneManager) do
        if type(v) == "function" then
            manager[k] = v
        end
    end
    manager:Init()
    return manager
end

return SceneManager