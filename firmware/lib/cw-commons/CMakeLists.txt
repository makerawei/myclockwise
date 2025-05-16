cmake_minimum_required(VERSION 3.5)

# ezTime
# This can be removed if https://github.com/ropg/ezTime/pull/157 is merged in.
set (
	 SRC_DIRS 
	 ${PROJECT_DIR}/firmware/lib/cw-commons/
	 ${PROJECT_DIR}/firmware/lib/cw-gfx-engine/
	 ${PROJECT_DIR}/components/Adafruit-GFX-Library/
	 ${PROJECT_DIR}/components/Adafruit_BusIO/
     ${PROJECT_DIR}/components/ezTime/src/
     ${PROJECT_DIR}/components/NTPClient/
     ${PROJECT_DIR}/components/ESP32Time/
     ${PROJECT_DIR}/components/ESP32-HUB75-MatrixPanel-I2S-DMA/src
	)
set (
	 INCLUDE_DIRS
	 ${PROJECT_DIR}/firmware/lib/cw-commons/
	 ${PROJECT_DIR}/firmware/lib/cw-gfx-engine/
	 ${PROJECT_DIR}/components/Adafruit-GFX-Library/
	 ${PROJECT_DIR}/components/Adafruit_BusIO/
     ${PROJECT_DIR}/components/ezTime/src/
     ${PROJECT_DIR}/components/NTPClient/
     ${PROJECT_DIR}/components/ESP32Time/
     ${PROJECT_DIR}/components/ESP32-HUB75-MatrixPanel-I2S-DMA/src
    )


idf_component_register(
                       INCLUDE_DIRS "." ${SRC_DIRS}
                       SRC_DIRS "." ${INCLUDE_DIRS}
                       REQUIRES arduino
                       )

project(cw-gfx-commons)
