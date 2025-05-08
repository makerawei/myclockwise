cmake_minimum_required(VERSION 3.5)

# ezTime
# This can be removed if https://github.com/ropg/ezTime/pull/157 is merged in.
set (
	 SRC_DIRS 
     ${PROJECT_DIR}/components/ezTime/src
     ${PROJECT_DIR}/components/NTPClient/
	)
set (
	 INCLUDE_DIRS
     ${PROJECT_DIR}/components/ezTime/src
     ${PROJECT_DIR}/components/NTPClient/
    )


idf_component_register(
                       INCLUDE_DIRS "." ${SRC_DIRS}
                       SRC_DIRS "." ${INCLUDE_DIRS}
                       REQUIRES arduino
                       )

project(cw-gfx-commons)
