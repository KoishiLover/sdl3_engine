add_executable(hatta-engine main.cpp
        engine/engine_frame.h
        engine/engine_frame.cpp
        engine/engine_window.hpp
        engine/engine_window.cpp
        engine/engine_log.hpp
        engine/engine_log.cpp
        engine/frame_controller.hpp
        engine/frame_controller.cpp
        engine/lua_bind.hpp
        engine/lua_bind.cpp
        engine/engine_utils.hpp
        engine/resource/Sprite.hpp
        engine/resource/Renderer.hpp
        engine/resource/Renderer.cpp
        engine/resource/Audio.cpp
        engine/resource/Audio.hpp
        engine/input/Input.hpp
        engine/input/Input.cpp
        engine/game/GameObject.cpp
        engine/game/GameObject.hpp
        engine/game/GameObjectManager.cpp
        engine/game/GameObjectManager.hpp
)
add_custom_command(TARGET hatta-engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/doc"
        "$<TARGET_FILE_DIR:hatta-engine>/doc"
        COMMENT "Copy LuaAPI doc"
)

target_link_libraries(hatta-engine PRIVATE
        SDL3::SDL3
        SDL3_image::SDL3_image
        SDL3_mixer::SDL3_mixer
        SDL3_ttf::SDL3_ttf
        spdlog::spdlog_header_only
        luajit::lib
        sol2::sol2
)

target_compile_definitions(hatta-engine PRIVATE SOL_LUAJIT=1)

if(WIN32)
    target_link_libraries(hatta-engine PRIVATE ws2_32 user32)
endif()
