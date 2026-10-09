---@meta
-----Hatta Engine 音频模块
-----@author 帽子屋小姐

---@param name string 资源名称
---@param path string 资源路径
---@param starts number 循环起点
---@param ends number 循环的终点
---@param loop number 循环的次数 ---必须是整数
----从路径 path 加载音乐资源，名称为 name
function hatta.LoadMusic(name,path,starts,ends,loop)
end

---@param name string 资源名称
----播放音乐资源
function hatta.PlayMusic(name)
end

---@param name string 资源名称
----停止播放音乐资源
function hatta.StopMusic(name)

end

---@param name string 资源名称
----暂停播放音乐
function hatta.PauseMusic(name)

end

---@param name string 资源名称
----将音乐恢复播放
function hatta.ResumeMusic(name)

end

---@param vol number 音量
----设置音乐音量
function hatta.SetMusicVolume(vol)

end

---@param name string 音效名称
---@param path string 音效路径
----从路径 path 加载 音效资源，名称为name
function hatta.LoadSE(name,path)

end

---@param name string 资源名称
----播放音效
function hatta.PlaySE(name)

end

---@param vol number
----设置音效音量
function hatta.SetSEVolume(vol)

end