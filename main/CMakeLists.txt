idf_component_register(SRCS "mk32_main.cpp" "keymap.c" "matrix.c" "keypress_handles.c"
                    INCLUDE_DIRS "."
                    REQUIRES "battery" "ble" "nvsfuncs" "plugins" "rotencoder" "ssd1306")
