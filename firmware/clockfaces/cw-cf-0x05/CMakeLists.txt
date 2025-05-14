cmake_minimum_required(VERSION 3.5)

set (
	 SRC_DIRS 
     ${PROJECT_DIR}/components/ArduinoJson/
     ${PROJECT_DIR}/components/PNGdec/src/
	)
set (
	 INCLUDE_DIRS
     ${PROJECT_DIR}/components/ArduinoJson/
     ${PROJECT_DIR}/components/PNGdec/src/
    )

if(CONFIG_CLOCKFACE_PACMAN)
  list(APPEND INCLUDE_DIRS ".")
  list(APPEND SRC_DIRS ".")
endif()

idf_component_register(
                       INCLUDE_DIRS ${INCLUDE_DIRS}
                       SRC_DIRS ${SRC_DIRS}
                       REQUIRES arduino Adafruit-GFX-Library cw-gfx-engine cw-commons ArduinoJson PNGdec
                       )
project(cw-cf-0x05)
