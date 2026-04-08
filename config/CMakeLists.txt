# Extra sources compiled into the ZMK app for this config.
# ZMK's build system picks this up automatically via -DZMK_CONFIG.
target_sources(app PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src/layer_reporter.c
    ${CMAKE_CURRENT_LIST_DIR}/src/custom_status_screen.c
)

# Our zmk_display_status_screen() (in libapp.a) and nice_view's (in libzephyr.a)
# are both strong symbols. libapp.a is linked first so --allow-multiple-definition
# keeps ours and silently drops nice_view's duplicate.
zephyr_ld_options(-Wl,--allow-multiple-definition)
