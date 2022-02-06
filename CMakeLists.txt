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

# find_package(date REQUIRED)

# TODO could use FetchContent for xtensor, dlfcn-win32, nlohmann_json, pybind11, etc.


### agpred executable ###

add_executable(agpred main.cpp "main_pred.cpp" "src/common.h" "src/common.cpp" "src/preprocess.h" "src/preprocess.cpp" "src/defs.h" "src/consolidate.h" "src/consolidate.cpp" "src/wsclient.cpp" "src/wsclient_ssl.cpp"
	"core/account.cpp" "core/account_controller.cpp" "core/data_controller.cpp" "core/position.cpp" "core/strategy.cpp" "adapters/polygon_io.cpp"
	)

target_include_directories(agpred PUBLIC ${xtensor_INCLUDE_DIRS} C:/ta-lib/c/include C:/share/OpenSSL-Win64-1.1.1l/include ${cpr_INCLUDE_DIRS} C:/src/tensorflow/bazel-bin/tensorflow/include)

# TensorflowCC::TensorflowCC
target_link_libraries(agpred PRIVATE cpr::cpr C:/share/OpenSSL-Win64-1.1.1l/lib/VC/libssl64MD.lib C:/share/OpenSSL-Win64-1.1.1l/lib/VC/libcrypto64MD.lib PUBLIC xtensor dlfcn-win32::dl C:/src/tensorflow/bazel-bin/tensorflow/libtensorflow_cc.so.2.5.2.if.lib C:/ta-lib/c/lib/ta_libc_cdr.lib C:/ta-lib/c/lib/ta_func_cdr.lib C:/ta-lib/c/lib/ta_common_cdr.lib C:/ta-lib/c/lib/ta_abstract_cdr.lib)
#  OpenSSL::applink OpenSSL::Crypto OpenSSL::SSL  CURL::libcurl    OpenSSL::applink
#  C:/share/cpr/lib/cpr.lib   date::date-tz 
# TensorflowCC::TensorflowCC
# target_link_libraries(agpred PUBLIC xtensor dlfcn-win32::dl C:/src/tensorflow/bazel-bin/tensorflow/libtensorflow_cc.so.2.5.2.if.lib OpenSSL::applink OpenSSL::Crypto OpenSSL::SSL C:/ta-lib/c/lib/ta_libc_cdr.lib C:/ta-lib/c/lib/ta_func_cdr.lib C:/ta-lib/c/lib/ta_common_cdr.lib C:/ta-lib/c/lib/ta_abstract_cdr.lib)



### Copy package assets into build dir ###
add_custom_command(TARGET agpred POST_BUILD        # Adds a post-build event to agpred
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
        "${cpr_BINARY_DIR}/cpr/cpr.dll"
        $<TARGET_FILE_DIR:agpred>)
#add_custom_command(TARGET agpred POST_BUILD        # Adds a post-build event to agpred
#    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
#        "${cpr_BINARY_DIR}/cpr/cpr.lib"
#        $<TARGET_FILE_DIR:agpred>)
add_custom_command(TARGET agpred POST_BUILD        # Adds a post-build event to agpred
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
        "${CURL_BINARY_DIR}/lib/libcurl.dll"
        $<TARGET_FILE_DIR:agpred>)
add_custom_command(TARGET agpred POST_BUILD        # Adds a post-build event to agpred
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
        "${zlib_BINARY_DIR}/zlib.dll"
        $<TARGET_FILE_DIR:agpred>)
#add_custom_command(TARGET agpred POST_BUILD        # Adds a post-build event to agpred
#    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
#        "${zlib_BINARY_DIR}/zlib.lib"
#        $<TARGET_FILE_DIR:agpred>)
# TODO libcrypto-1_1-x64.dll libssl-1_1-x64.dll cacert.pem
# TODO dl.lib ?


### agpred_sys executable ###

add_executable(agpred_sys main_sys.cpp "src/common.h" "src/common.cpp" "src/preprocess.h" "src/preprocess.cpp" "src/defs.h" "src/consolidate.h" "src/consolidate.cpp" "src/wsclient.cpp" "src/wsclient_ssl.cpp"
	"core/account.cpp" "core/account_controller.cpp" "core/data_controller.cpp" "core/position.cpp" "core/strategy.cpp" "adapters/polygon_io.cpp"
	)

target_include_directories(agpred_sys PUBLIC ${xtensor_INCLUDE_DIRS} C:/ta-lib/c/include C:/share/OpenSSL-Win64-1.1.1l/include ${cpr_INCLUDE_DIRS})

# TensorflowCC::TensorflowCC
target_link_libraries(agpred_sys PRIVATE cpr::cpr C:/share/OpenSSL-Win64-1.1.1l/lib/VC/libssl64MD.lib C:/share/OpenSSL-Win64-1.1.1l/lib/VC/libcrypto64MD.lib PUBLIC xtensor dlfcn-win32::dl C:/src/tensorflow/bazel-bin/tensorflow/libtensorflow_cc.so.2.5.2.if.lib C:/ta-lib/c/lib/ta_libc_cdr.lib C:/ta-lib/c/lib/ta_func_cdr.lib C:/ta-lib/c/lib/ta_common_cdr.lib C:/ta-lib/c/lib/ta_abstract_cdr.lib)



### agpred_wss executable ###

add_executable(agpred_wss main_wss.cpp 
	"src/common.cpp" "src/preprocess.cpp" "src/consolidate.cpp" "src/wsclient.cpp" "src/wsclient_ssl.cpp"
	"core/account.cpp" "core/account_controller.cpp" "core/data_controller.cpp" "core/position.cpp" "core/strategy.cpp" "adapters/polygon_io.cpp"
	)

target_include_directories(agpred_wss PUBLIC ${xtensor_INCLUDE_DIRS} C:/ta-lib/c/include C:/share/OpenSSL-Win64-1.1.1l/include ${cpr_INCLUDE_DIRS})

# TensorflowCC::TensorflowCC
target_link_libraries(agpred_wss PRIVATE cpr::cpr C:/share/OpenSSL-Win64-1.1.1l/lib/VC/libssl64MD.lib C:/share/OpenSSL-Win64-1.1.1l/lib/VC/libcrypto64MD.lib PUBLIC xtensor dlfcn-win32::dl C:/src/tensorflow/bazel-bin/tensorflow/libtensorflow_cc.so.2.5.2.if.lib C:/ta-lib/c/lib/ta_libc_cdr.lib C:/ta-lib/c/lib/ta_func_cdr.lib C:/ta-lib/c/lib/ta_common_cdr.lib C:/ta-lib/c/lib/ta_abstract_cdr.lib)



### agpred_dl executable ###

add_executable(agpred_dl main_dl.cpp "src/common.h" "src/common.cpp" "src/preprocess.h" "src/preprocess.cpp" "src/defs.h" "src/consolidate.h" "src/consolidate.cpp" "src/wsclient.cpp" "src/wsclient_ssl.cpp"
	"core/account.cpp" "core/account_controller.cpp" "core/data_controller.cpp" "core/position.cpp" "core/strategy.cpp" "adapters/polygon_io.cpp"
	)

target_include_directories(agpred_dl PUBLIC ${xtensor_INCLUDE_DIRS} C:/ta-lib/c/include C:/src/pybind11/include C:/share/Python39/include C:/share/Python39/Lib/site-packages/numpy/core/include C:/share/OpenSSL-Win64-1.1.1l/include ${cpr_INCLUDE_DIRS})

# TensorflowCC::TensorflowCC
target_link_libraries(agpred_dl PRIVATE cpr::cpr C:/share/OpenSSL-Win64-1.1.1l/lib/VC/libssl64MD.lib C:/share/OpenSSL-Win64-1.1.1l/lib/VC/libcrypto64MD.lib PUBLIC xtensor dlfcn-win32::dl C:/src/tensorflow/bazel-bin/tensorflow/libtensorflow_cc.so.2.5.2.if.lib Python::Python C:/ta-lib/c/lib/ta_libc_cdr.lib C:/ta-lib/c/lib/ta_func_cdr.lib C:/ta-lib/c/lib/ta_common_cdr.lib C:/ta-lib/c/lib/ta_abstract_cdr.lib)



### test_kml executable ###

add_executable(test_kml "test_kml.cu"
	"src/data_generator.cpp" 
	"src/common.h" "src/common.cpp" "src/preprocess.h" "src/preprocess.cpp" "src/defs.h" "src/consolidate.h" "src/consolidate.cpp" "src/wsclient.cpp" "src/wsclient_ssl.cpp"
	"core/account.cpp" "core/account_controller.cpp" "core/data_controller.cpp" "core/position.cpp" "core/strategy.cpp" "adapters/polygon_io.cpp"
	"deps/glob/glob_win32.cpp"
	)

target_include_directories(test_kml PUBLIC ${xtensor_INCLUDE_DIRS} ${CUDAToolkit_INCLUDE_DIRS} C:/ta-lib/c/include C:/src/pybind11/include C:/share/Python39/include C:/share/Python39/Lib/site-packages/numpy/core/include C:/share/OpenSSL-Win64-1.1.1l/include ${cpr_INCLUDE_DIRS} C:/src/tiny-cuda-nn/include C:/src/tiny-cuda-nn/dependencies)

# TensorflowCC::TensorflowCC
target_link_libraries(test_kml PRIVATE xtensor cpr::cpr C:/share/OpenSSL-Win64-1.1.1l/lib/VC/libssl64MD.lib C:/share/OpenSSL-Win64-1.1.1l/lib/VC/libcrypto64MD.lib PUBLIC CUDA::cuda_driver CUDA::cudart CUDA::cublas CUDA::curand dlfcn-win32::dl C:/src/tensorflow/bazel-bin/tensorflow/libtensorflow_cc.so.2.5.2.if.lib Python::Python C:/ta-lib/c/lib/ta_libc_cdr.lib C:/ta-lib/c/lib/ta_func_cdr.lib C:/ta-lib/c/lib/ta_common_cdr.lib C:/ta-lib/c/lib/ta_abstract_cdr.lib C:/src/tiny-cuda-nn/build/src/Release/tiny-cuda-nn.lib)

target_compile_options(test_kml PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:${CUDA_NVCC_FLAGS}>)
# target_compile_options(test_kml PRIVATE "/O3")
#message(STATUS "updating options for target: " test_kml)



### pybind11 module ###

pybind11_add_module(agen_xt_api "src/agen_xt_api.cpp" "src/consolidate.cpp" "src/preprocess.cpp")

target_include_directories(agen_xt_api PUBLIC ${xtensor_INCLUDE_DIRS} C:/ta-lib/c/include C:/share/Python39/Lib/site-packages/numpy/core/include)

target_link_libraries(agen_xt_api PUBLIC xtensor C:/ta-lib/c/lib/ta_libc_cdr.lib C:/ta-lib/c/lib/ta_func_cdr.lib C:/ta-lib/c/lib/ta_common_cdr.lib C:/ta-lib/c/lib/ta_abstract_cdr.lib)



