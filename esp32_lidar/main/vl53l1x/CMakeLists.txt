idf_component_register(
    SRCS
        "VL53L1X_api.c"
        "VL53L1X_calibration.c"
        "vl53l1_platform.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES driver freertos
)
