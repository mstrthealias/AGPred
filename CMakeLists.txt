cmake_minimum_required(VERSION 3.17)
project(agpred LANGUAGES CXX CUDA)


if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_CRT_SECURE_NO_WARNINGS")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_PREFIX_PATH c:/share/xtensor;C:/src/tensorflow/bazel-bin;C:/ta-lib/c/lib;C:/share/OpenSSL-Win64-1.1.1l;C:/share/stldate)
# ;C:/share/curl-7.79.1  # Note: cpr includes zlib/curl
# ;C:/share/cpr  # Note using FetchContent



###############################################################################
# CUDA compiler setup
###############################################################################

set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CUDA_LINK_LIBRARIES_KEYWORD PUBLIC)

include(FindCUDA/select_compute_arch)
cuda_select_nvcc_arch_flags(ARCH_FLAGS)
list(APPEND CUDA_NVCC_FLAGS ${ARCH_FLAGS})

set(CMAKE_CUDA_ARCHITECTURES off)

#if (MSVC)
#else()
#	list(APPEND CUDA_NVCC_FLAGS "-Xcompiler=-mf16c")
#	list(APPEND CUDA_NVCC_FLAGS "-Xcompiler=-Wconversion")
#	list(APPEND CUDA_NVCC_FLAGS "-Xcompiler=-fno-strict-aliasing")
#endif()
list(APPEND CUDA_NVCC_FLAGS "--extended-lambda")
list(APPEND CUDA_NVCC_FLAGS "--expt-relaxed-constexpr")

# TODO verify:
## Optimize the CUDA build
#list(APPEND CUDA_NVCC_FLAGS "-Xptxas -O3,-v")

###############################################################################
# Include files
###############################################################################

#if (MSVC)
#else()
#	set(CUDA_TOOLKIT_ROOT_DIR /opt/cuda/targets/x86_64-linux)
#endif()
find_library(
	CUDA_CUBLASLT_LIBRARY cublasLt
	${CUDA_TOOLKIT_ROOT_DIR}/lib64
	${CUDA_TOOLKIT_ROOT_DIR}/lib
)
#include_directories("include")
#include_directories("dependencies")



###############################################################################
# Packages
###############################################################################

find_package(CUDAToolkit 11.6 REQUIRED)

find_package(dlfcn-win32 REQUIRED)
find_package(TensorflowCC REQUIRED)
find_package(xtensor REQUIRED)  # add_subdirectory(c:/share/xtensor)
find_package(nlohmann_json REQUIRED)

find_package(Python COMPONENTS Interpreter Development REQUIRED)  # TODO remove Development?
find_package(pybind11 CONFIG REQUIRED)  # add_subdirectory(C:/src/pybind11 pybind11_build)

find_package(OpenSSL REQUIRED)
# find_package(CURL REQUIRED)  # CPR includes CURL

# TODO could do this:
set(CPR_BUILD_TESTS_SSL 0)
include(FetchContent)
FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git GIT_TAG f4622efcb59d84071ae11404ae61bd821c1c344b) # the commit hash for 1.6.2
FetchContent_MakeAvailable(cpr)
set(cpr_INCLUDE_DIRS ${cpr_SOURCE_DIR}/include ${cpr_SOURCE_DIR}/../curl-src/include)

# find_package(date REQUIRED)

# TODO could use FetchContent for xtensor, dlfcn-win32, nlohmann_json, pybind11, etc.


###############################################################################
# Misc. Variables
###############################################################################

set(TENSORFLOW_LIBS "C:/src/tensorflow/bazel-bin/tensorflow/tensorflow.dll.if.lib")  # "TensorflowCC::TensorflowCC" "libtensorflow_cc.so.2.5.2.if.lib"
set(TALIB_LIBS "C:/ta-lib/c/lib/ta_libc_cdr.lib" "C:/ta-lib/c/lib/ta_func_cdr.lib" "C:/ta-lib/c/lib/ta_common_cdr.lib" "C:/ta-lib/c/lib/ta_abstract_cdr.lib")
set(OPENSSL_LIBS "C:/share/OpenSSL-Win64-1.1.1l/lib/VC/libssl64MD.lib" "C:/share/OpenSSL-Win64-1.1.1l/lib/VC/libcrypto64MD.lib")
set(XTENSOR_LIBS "xtensor" "dlfcn-win32::dl")
set(CPR_LIBS "cpr::cpr")
set(CUDA_LIBS "CUDA::cuda_driver" "CUDA::cublas" "CUDA::curand")  # "CUDA::cudart"


###############################################################################
# Project Includes
###############################################################################


include_directories(${xtensor_INCLUDE_DIRS})
include_directories(C:/ta-lib/c/include)
include_directories(C:/share/OpenSSL-Win64-1.1.1l/include)
include_directories(${cpr_INCLUDE_DIRS})
include_directories(C:/src/tensorflow/bazel-bin/tensorflow/include)



###############################################################################
# Targets
###############################################################################

### agpred_process executable ###

add_executable(agpred_process "main_process.cpp" "src/common.cpp" "src/preprocess.cpp" "src/consolidate.cpp" "src/wsclient.cpp" "src/wsclient_ssl.cpp" "src/util.cpp"
	"core/core.cpp" "core/account.cpp" "core/account_controller.cpp" "core/data_controller.cpp" "core/position.cpp" "core/strategy.cpp" "adapters/polygon_io.cpp"
	)

target_link_libraries(agpred_process PRIVATE ${CPR_LIBS} ${OPENSSL_LIBS} PUBLIC ${XTENSOR_LIBS} ${TENSORFLOW_LIBS} ${TALIB_LIBS})
#  OpenSSL::applink OpenSSL::Crypto OpenSSL::SSL  CURL::libcurl    OpenSSL::applink
#  C:/share/cpr/lib/cpr.lib   date::date-tz 
# target_link_libraries(agpred_process PUBLIC ${XTENSOR_LIBS} ${TENSORFLOW_LIBS} OpenSSL::applink OpenSSL::Crypto OpenSSL::SSL ${TALIB_LIBS})



### Copy package assets into build dir ###
add_custom_command(TARGET agpred_process POST_BUILD        # Adds a post-build event to agpred_process
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
        "${cpr_BINARY_DIR}/cpr/cpr.dll"
        $<TARGET_FILE_DIR:agpred_process>)
add_custom_command(TARGET agpred_process POST_BUILD        # Adds a post-build event to agpred_process
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
        "${CURL_BINARY_DIR}/lib/libcurl.dll"
        $<TARGET_FILE_DIR:agpred_process>)
add_custom_command(TARGET agpred_process POST_BUILD        # Adds a post-build event to agpred_process
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
        "${zlib_BINARY_DIR}/zlib.dll"
        $<TARGET_FILE_DIR:agpred_process>)
# TODO libcrypto-1_1-x64.dll libssl-1_1-x64.dll cacert.pem



### agpred_pred executable ###

add_executable(agpred_pred "main_pred.cpp" "src/common.cpp" "src/preprocess.cpp" "src/consolidate.cpp" "src/wsclient.cpp" "src/wsclient_ssl.cpp" "src/util.cpp"
	"core/core.cpp" "core/account.cpp" "core/account_controller.cpp" "core/data_controller.cpp" "core/position.cpp" "core/strategy.cpp" "adapters/polygon_io.cpp"
	)

target_link_libraries(agpred_pred
									PRIVATE ${CPR_LIBS} ${OPENSSL_LIBS}
									PUBLIC ${XTENSOR_LIBS} 
											${TENSORFLOW_LIBS} 
											${TALIB_LIBS}
					)



### agpred_sys executable ###

add_executable(agpred_sys main_sys.cpp "src/common.cpp" "src/preprocess.cpp" "src/consolidate.cpp" "src/wsclient.cpp" "src/wsclient_ssl.cpp" "src/util.cpp"
	"core/core.cpp" "core/account.cpp" "core/account_controller.cpp" "core/data_controller.cpp" "core/position.cpp" "core/strategy.cpp" "adapters/polygon_io.cpp"
	)

target_link_libraries(agpred_sys PRIVATE ${CPR_LIBS} ${OPENSSL_LIBS} PUBLIC ${XTENSOR_LIBS} ${TENSORFLOW_LIBS} ${TALIB_LIBS})



### agpred_wss executable ###

add_executable(agpred_wss main_wss.cpp 
	"src/common.cpp" "src/preprocess.cpp" "src/consolidate.cpp" "src/wsclient.cpp" "src/wsclient_ssl.cpp" "src/util.cpp"
	"core/core.cpp" "core/account.cpp" "core/account_controller.cpp" "core/data_controller.cpp" "core/position.cpp" "core/strategy.cpp"
	"adapters/polygon_io.cpp" "algos/ma3_ema9.cpp"
	)

target_link_libraries(agpred_wss PRIVATE ${CPR_LIBS} ${OPENSSL_LIBS} PUBLIC ${XTENSOR_LIBS} ${TENSORFLOW_LIBS} ${TALIB_LIBS})



### agpred_dl executable ###

add_executable(agpred_dl main_dl.cpp "src/common.cpp" "src/preprocess.cpp" "src/consolidate.cpp" "src/wsclient.cpp" "src/wsclient_ssl.cpp" "src/util.cpp"
	"core/core.cpp" "core/account.cpp" "core/account_controller.cpp" "core/data_controller.cpp" "core/position.cpp" "core/strategy.cpp"
	"adapters/polygon_io.cpp" "algos/ma3_ema9.cpp" "sim/downloader.cpp"
	)

target_link_libraries(agpred_dl PRIVATE ${CPR_LIBS} ${OPENSSL_LIBS} PUBLIC ${XTENSOR_LIBS} ${TENSORFLOW_LIBS} ${TALIB_LIBS})



### agpred_backtest executable ###

add_executable(agpred_backtest main_backtest.cpp "src/common.cpp" "src/preprocess.cpp" "src/consolidate.cpp" "src/wsclient.cpp" "src/wsclient_ssl.cpp" "src/util.cpp"
	"core/core.cpp" "core/account.cpp" "core/account_controller.cpp" "core/data_controller.cpp" "core/position.cpp" "core/strategy.cpp"
	"adapters/polygon_io.cpp"
	"algos/ma3_ema9.cpp" "algos/long_low.cpp"
	)

target_link_libraries(agpred_backtest PRIVATE ${CPR_LIBS} ${OPENSSL_LIBS} PUBLIC ${XTENSOR_LIBS} ${TENSORFLOW_LIBS} ${TALIB_LIBS})



### test_kml executable ###

add_executable(test_kml "test_kml.cu"
	"src/data_generator.cpp" 
	"src/common.cpp" "src/preprocess.cpp" "src/consolidate.cpp" "src/util.cpp"
	"deps/glob/glob_win32.cpp"
	)

target_include_directories(test_kml PUBLIC C:/src/tiny-cuda-nn/include C:/src/tiny-cuda-nn/dependencies)

target_link_libraries(test_kml PUBLIC ${XTENSOR_LIBS} ${CUDA_LIBS} ${TALIB_LIBS} C:/src/tiny-cuda-nn/build/src/Release/tiny-cuda-nn.lib)

target_compile_options(test_kml PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:${CUDA_NVCC_FLAGS}>)
# target_compile_options(test_kml PRIVATE "/O3")
#message(STATUS "updating options for target: " test_kml)



### pybind11 module ###

pybind11_add_module(agen_xt_api "src/agen_xt_api.cpp" "src/common.cpp" "src/consolidate.cpp" "src/preprocess.cpp" "src/util.cpp")

target_include_directories(agen_xt_api PUBLIC C:/share/Python39/Lib/site-packages/numpy/core/include C:/src/pybind11/include C:/share/Python39/include C:/share/Python39/Lib/site-packages/numpy/core/include)

target_link_libraries(agen_xt_api PUBLIC ${XTENSOR_LIBS} ${TALIB_LIBS})



### agpred library ###

add_library(agpred_library STATIC library.cpp "src/common.cpp" "src/preprocess.cpp" "src/consolidate.cpp" "src/wsclient.cpp" "src/wsclient_ssl.cpp" "src/util.cpp"
	"core/core.cpp" "core/account.cpp" "core/account_controller.cpp" "core/data_controller.cpp" "core/position.cpp" "core/strategy.cpp"
	"adapters/polygon_io.cpp"
	"algos/ma3_ema9.cpp" "algos/long_low.cpp"
	)
	
#set_property(TARGET agpred_library PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_property(TARGET agpred_library PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
#set_property(TARGET agpred_library PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
#set_property(TARGET agpred_library PROPERTY MSVC_RUNTIME_LIBRARY "")

#target_link_libraries(agpred_library PRIVATE ${CPR_LIBS} ${OPENSSL_LIBS} ${XTENSOR_LIBS} ${TENSORFLOW_LIBS} ${TALIB_LIBS})


