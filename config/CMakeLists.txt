# Extra sources compiled into the ZMK app for this config.
# ZMK's build system picks this up automatically via -DZMK_CONFIG.
target_sources(app PRIVATE src/layer_reporter.c)
