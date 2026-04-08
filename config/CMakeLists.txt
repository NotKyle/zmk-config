# Extra sources compiled into the ZMK app for this config.
# ZMK's build system picks this up automatically via -DZMK_CONFIG.
target_sources(app PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src/layer_reporter.c
    ${CMAKE_CURRENT_LIST_DIR}/src/custom_status_screen.c
)
