cmake_minimum_required(VERSION 3.10)

project(tvm-live)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/target)

# the `pkg_check_modules` function is created with this call
find_package(PkgConfig REQUIRED)

pkg_check_modules(OPENCV REQUIRED IMPORTED_TARGET opencv)
pkg_check_modules(GST REQUIRED IMPORTED_TARGET gstreamer-1.0)
pkg_check_modules(GST_APP REQUIRED IMPORTED_TARGET gstreamer-app-1.0)
pkg_check_modules(DRM REQUIRED IMPORTED_TARGET libdrm)
pkg_check_modules(DRM_OMAP REQUIRED IMPORTED_TARGET libdrm_omap)
pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(SRC "src")
set(TI_ACC "src/ti-acc")
set(PERF_STATS "src/ti-acc/utils/perf_stats")
set(REMOTE_SERVICE "src/ti-acc/utils/remote_service")
set(IPC "src/ti-acc/utils/ipc")
set(CONSOLE_IO "src/ti-acc/utils/console_io")

add_executable(tvm-live
	"${SRC}/app/tvm-live.cpp"
	"${SRC}/capdisplay/ticapturedisplay.cpp"
	"${SRC}/app/tvm_app.cpp"
	"${SRC}/model/neomodel.cpp"
	"${SRC}/model/neosample.cpp"
	"${SRC}/gui/stats.cpp"
	"${SRC}/gui/camgui.cpp"
	"${SRC}/gui/gstgui.cpp"
	"${SRC}/gui/abstractgui.cpp"
	"${SRC}/controller/phidgetcontroller.cpp"
	"${SRC}/controller/controller.cpp"
	"${SRC}/app/autonomous.cpp"
	"${SRC}/model/abstractmodel.cpp"
    	"${SRC}/capdisplay/gstcapturedisplay.cpp"

	"${TI_ACC}/cmem_buf.cpp"
	"${TI_ACC}/disp_obj.cpp"
	"${TI_ACC}/vip_obj.cpp"
	"${TI_ACC}/vpe_obj.cpp"
	"${TI_ACC}/save_utils.cpp"

	# "${PERF_STATS}/src/app_perf_stats_api.c"
	# "${PERF_STATS}/src/app_perf_stats_linux.c"
	
	# "${REMOTE_SERVICE}/src/app_remote_service_linux.c"

	# "${IPC}/src/app_ipc_linux.c"
	# "${IPC}/src/app_ipc_linux_rpmsg_char.c"

	# "${CONSOLE_IO}/src/app_log_linux.c"
	# "${CONSOLE_IO}/src/app_log_writer.c"
	)

target_include_directories(tvm-live PUBLIC
	"./src"
	"./src/ti-acc/"
	"./include")

      
find_library(DLR_LIBRARY dlr HINTS ./lib/)
# find_library(TVM_LIBRARY tvm_runtime_static HINTS ../neo-ai-dlr/build/3rdparty/tvm)
# find_library(TREELITE_LIBRARY treelite_runtime_static HINTS ../neo-ai-dlr/build/lib)

target_link_libraries(tvm-live PUBLIC
	Threads::Threads
	PkgConfig::OPENCV
  	PkgConfig::GST
  	PkgConfig::GST_APP
	PkgConfig::DRM_OMAP
	PkgConfig::DRM
	PkgConfig::CAIRO
	${DLR_LIBRARY}
	ticmem
	phidget22)

file(	COPY ${CMAKE_CURRENT_SOURCE_DIR}/models
	DESTINATION ${CMAKE_BINARY_DIR}/target)

file(	COPY ${CMAKE_CURRENT_SOURCE_DIR}/video.mkv
	DESTINATION ${CMAKE_BINARY_DIR}/target)

file(	COPY ${CMAKE_CURRENT_SOURCE_DIR}/stops.txt
	DESTINATION ${CMAKE_BINARY_DIR}/target)
      
file(	GLOB TARGET_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/scripts/*)

file(	COPY ${TARGET_SCRIPTS}
	DESTINATION ${CMAKE_BINARY_DIR}/target)


set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

