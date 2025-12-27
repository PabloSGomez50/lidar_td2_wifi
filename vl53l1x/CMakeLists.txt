add_library(vl53l1x
    vl53l1x.c
    VL53L1X_api.c
    VL53L1X_calibration.c
    vl53l1_platform.c
)

target_include_directories(vl53l1x PUBLIC
	.
)

target_link_libraries(vl53l1x pico_stdlib hardware_i2c)