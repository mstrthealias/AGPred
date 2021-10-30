cmake_minimum_required(VERSION 3.15)
project(agpred LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_PREFIX_PATH c:/share/xtensor;C:/src/tensorflow/bazel-bin;C:/ta-lib/c/lib;C:/share/OpenSSL-Win64-1.1.1l;C:/share/stldate)
# ;C:/share/curl-7.79.1  # Note: cpr includes zlib/curl
# ;C:/share/cpr  # Note using FetchContent


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


### agpred executable ###

add_executable(agpred main.cpp "main_pred.cpp" "src/common.h" "src/common.cpp" "src/preprocess.h" "src/preprocess.cpp" "src/defs.h" "src/consolidate.h" "src/consolidate.cpp" "src/wsclient.cpp" "src/wsclient_ssl.cpp"
	"core/account.cpp" "core/account_controller.cpp" "core/data_controller.cpp" "core/position.cpp" "core/strategy.cpp" "adapters/polygon_io.cpp"
	)

target_include_directories(agpred PUBLIC ${xtensor_INCLUDE_DIRS} C:/ta-lib/c/include C:/share/OpenSSL-Win64-1.1.1l/include C:/share/cpr/include)

target_link_libraries(agpred PRIVATE cpr::cpr C:/share/OpenSSL-Win64-1.1.1l/lib/VC/libssl64MD.lib C:/share/OpenSSL-Win64-1.1.1l/lib/VC/libcrypto64MD.lib PUBLIC xtensor dlfcn-win32::dl TensorflowCC::TensorflowCC C:/ta-lib/c/lib/ta_libc_cdr.lib C:/ta-lib/c/lib/ta_func_cdr.lib C:/ta-lib/c/lib/ta_common_cdr.lib C:/ta-lib/c/lib/ta_abstract_cdr.lib)
#  OpenSSL::applink OpenSSL::Crypto OpenSSL::SSL  CURL::libcurl    OpenSSL::applink
#  C:/share/cpr/lib/cpr.lib   date::date-tz 
# target_link_libraries(agpred PUBLIC xtensor dlfcn-win32::dl TensorflowCC::TensorflowCC OpenSSL::applink OpenSSL::Crypto OpenSSL::SSL C:/ta-lib/c/lib/ta_libc_cdr.lib C:/ta-lib/c/lib/ta_func_cdr.lib C:/ta-lib/c/lib/ta_common_cdr.lib C:/ta-lib/c/lib/ta_abstract_cdr.lib)



### agpred_sys executable ###

add_executable(agpred_sys main_sys.cpp "src/common.h" "src/common.cpp" "src/preprocess.h" "src/preprocess.cpp" "src/defs.h" "src/consolidate.h" "src/consolidate.cpp" "src/wsclient.cpp" "src/wsclient_ssl.cpp"
	"core/account.cpp" "core/account_controller.cpp" "core/data_controller.cpp" "core/position.cpp" "core/strategy.cpp" "adapters/polygon_io.cpp"
	)

target_include_directories(agpred_sys PUBLIC ${xtensor_INCLUDE_DIRS} C:/ta-lib/c/include C:/share/OpenSSL-Win64-1.1.1l/include C:/share/cpr/include)

target_link_libraries(agpred_sys PRIVATE cpr::cpr C:/share/OpenSSL-Win64-1.1.1l/lib/VC/libssl64MD.lib C:/share/OpenSSL-Win64-1.1.1l/lib/VC/libcrypto64MD.lib PUBLIC xtensor dlfcn-win32::dl TensorflowCC::TensorflowCC C:/ta-lib/c/lib/ta_libc_cdr.lib C:/ta-lib/c/lib/ta_func_cdr.lib C:/ta-lib/c/lib/ta_common_cdr.lib C:/ta-lib/c/lib/ta_abstract_cdr.lib)



### agpred_wss executable ###

add_executable(agpred_wss main_wss.cpp 
	"src/common.cpp" "src/preprocess.cpp" "src/consolidate.cpp" "src/wsclient.cpp" "src/wsclient_ssl.cpp"
	"core/account.cpp" "core/account_controller.cpp" "core/data_controller.cpp" "core/position.cpp" "core/strategy.cpp" "adapters/polygon_io.cpp"
	)

target_include_directories(agpred_wss PUBLIC ${xtensor_INCLUDE_DIRS} C:/ta-lib/c/include C:/share/OpenSSL-Win64-1.1.1l/include C:/share/cpr/include)

target_link_libraries(agpred_wss PRIVATE cpr::cpr C:/share/OpenSSL-Win64-1.1.1l/lib/VC/libssl64MD.lib C:/share/OpenSSL-Win64-1.1.1l/lib/VC/libcrypto64MD.lib PUBLIC xtensor dlfcn-win32::dl TensorflowCC::TensorflowCC C:/ta-lib/c/lib/ta_libc_cdr.lib C:/ta-lib/c/lib/ta_func_cdr.lib C:/ta-lib/c/lib/ta_common_cdr.lib C:/ta-lib/c/lib/ta_abstract_cdr.lib)



### agpred_dl executable ###

add_executable(agpred_dl main_dl.cpp "src/common.h" "src/common.cpp" "src/preprocess.h" "src/preprocess.cpp" "src/defs.h" "src/consolidate.h" "src/consolidate.cpp" "src/wsclient.cpp" "src/wsclient_ssl.cpp"
	"core/account.cpp" "core/account_controller.cpp" "core/data_controller.cpp" "core/position.cpp" "core/strategy.cpp" "adapters/polygon_io.cpp"
	)

target_include_directories(agpred_dl PUBLIC ${xtensor_INCLUDE_DIRS} C:/ta-lib/c/include C:/src/pybind11/include C:/share/Python39/include C:/share/Python39/Lib/site-packages/numpy/core/include C:/share/OpenSSL-Win64-1.1.1l/include C:/share/cpr/include)

target_link_libraries(agpred_dl PRIVATE cpr::cpr C:/share/OpenSSL-Win64-1.1.1l/lib/VC/libssl64MD.lib C:/share/OpenSSL-Win64-1.1.1l/lib/VC/libcrypto64MD.lib PUBLIC xtensor dlfcn-win32::dl TensorflowCC::TensorflowCC Python::Python C:/ta-lib/c/lib/ta_libc_cdr.lib C:/ta-lib/c/lib/ta_func_cdr.lib C:/ta-lib/c/lib/ta_common_cdr.lib C:/ta-lib/c/lib/ta_abstract_cdr.lib)



### pybind11 module ###

pybind11_add_module(agen_xt_api "src/agen_xt_api.cpp" "src/consolidate.cpp" "src/preprocess.cpp")

target_include_directories(agen_xt_api PUBLIC ${xtensor_INCLUDE_DIRS} C:/ta-lib/c/include C:/share/Python39/Lib/site-packages/numpy/core/include)

target_link_libraries(agen_xt_api PUBLIC xtensor C:/ta-lib/c/lib/ta_libc_cdr.lib C:/ta-lib/c/lib/ta_func_cdr.lib C:/ta-lib/c/lib/ta_common_cdr.lib C:/ta-lib/c/lib/ta_abstract_cdr.lib)



