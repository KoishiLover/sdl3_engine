## luajit2
set(LUAJIT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/luajit2/src")
set(LUAJIT_LIB "${LUAJIT_SOURCE_DIR}/lua51.lib")
add_library(luajit STATIC IMPORTED GLOBAL)
set_target_properties(luajit PROPERTIES
        IMPORTED_LOCATION "${LUAJIT_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${LUAJIT_SOURCE_DIR}"
)
add_library(luajit::lib ALIAS luajit)
