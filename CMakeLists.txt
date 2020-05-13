# CMake 最低版本
cmake_minimum_required(VERSION 2.6)

# 项目名称
project(muduo C CXX)

# 启用对此目录和下级目录的测试
enable_testing()

# 定义编译类型
# 实际的设置放在了 build.sh 脚本中
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

# only build examples if this is the main project
# STREQUAL 用于比较字符串，相同返回 true
if(CMAKE_PROJECT_NAME STREQUAL "muduo")
# 如果用户没有定义过，那默认值就是ON
# 如果用户在命令行显示改变过比如为OFF,那就是OFF
  option(MUDUO_BUILD_EXAMPLES "Build Muduo examples" ON)
endif()

# g++ 编译选项
set(CXX_FLAGS
# 方便gbd调试
 -g
 # D 是宏
 # 方便valgrind
 # -DVALGRIND
 # 检查线程返回值
 -DCHECK_PTHREAD_RETURN_VALUE
# 用64位地址可以防止打开大文件
 -D_FILE_OFFSET_BITS=64
# 启用大部分警告信息
 -Wall
# 打印额外的警告信息
 -Wextra
# 所有警告都会报错并停止编译
 -Werror
# 当隐式类型转换可能改变值时给出警告
 -Wconversion
# 未使用变量
 -Wno-unused-parameter
# 使用c风格的转化
 -Wold-style-cast
# 重载虚函数
 -Woverloaded-virtual
# 对函数指针或者void *类型的指针进行算术操作
 -Wpointer-arith
# 局部变量改写警告
 -Wshadow
# c风格常量字符串警告
 -Wwrite-strings
# 指令集优化
 -march=native
 # 自动生成依赖
 # -MMD
 # c++11 标准
 -std=c++11
# 用来通知链接器将所有添加到动态符号表中
 -rdynamic
 )

# 判断寄存器位数
if(CMAKE_BUILD_BITS EQUAL 32)
# 设定32位
  list(APPEND CXX_FLAGS "-m32")
endif()

# 判断编译器
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  list(APPEND CXX_FLAGS "-Wno-null-dereference")
  list(APPEND CXX_FLAGS "-Wno-sign-conversion")
  list(APPEND CXX_FLAGS "-Wno-unused-local-typedef")
  list(APPEND CXX_FLAGS "-Wthread-safety")
  list(REMOVE_ITEM CXX_FLAGS "-rdynamic")
endif()

# ${} 取值
# 使用引号会把字符间插入;分号,否则就啥都没有连在一起
# string(REPLACE <match_string>
#        <replace_string> <output_variable>
#        <input> [<input>...])
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

# 设置变量
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

# 如果没找到设有 REQUIRED 选项会报错停止
find_package(Boost REQUIRED)
find_package(Protobuf)
find_package(CURL)
find_package(ZLIB)

find_path(CARES_INCLUDE_DIR ares.h)
find_library(CARES_LIBRARY NAMES cares)
find_path(MHD_INCLUDE_DIR microhttpd.h)
find_library(MHD_LIBRARY NAMES microhttpd)
find_library(BOOSTTEST_LIBRARY NAMES boost_unit_test_framework)
find_library(BOOSTPO_LIBRARY NAMES boost_program_options)
find_library(BOOSTSYSTEM_LIBRARY NAMES boost_system)
find_path(TCMALLOC_INCLUDE_DIR gperftools/heap-profiler.h)
find_library(TCMALLOC_LIBRARY NAMES tcmalloc_and_profiler)
find_path(HIREDIS_INCLUDE_DIR hiredis/hiredis.h)
find_library(HIREDIS_LIBRARY NAMES hiredis)
find_path(GD_INCLUDE_DIR gd.h)
find_library(GD_LIBRARY NAMES gd)
find_program(THRIFT_COMPILER thrift)
find_path(THRIFT_INCLUDE_DIR thrift)
find_library(THRIFT_LIBRARY NAMES thrift)

if(CARES_INCLUDE_DIR AND CARES_LIBRARY)
  message(STATUS "found cares")
endif()
if(CURL_FOUND)
  message(STATUS "found curl")
endif()
if(PROTOBUF_FOUND)
  message(STATUS "found protobuf")
endif()
if(TCMALLOC_INCLUDE_DIR AND TCMALLOC_LIBRARY)
  message(STATUS "found tcmalloc")
endif()
if(ZLIB_FOUND)
  message(STATUS "found zlib")
endif()
if(HIREDIS_INCLUDE_DIR AND HIREDIS_LIBRARY)
  message(STATUS "found hiredis")
endif()
if(GD_INCLUDE_DIR AND GD_LIBRARY)
  message(STATUS "found gd")
endif()
if(THRIFT_COMPILER AND THRIFT_INCLUDE_DIR AND THRIFT_LIBRARY)
  message(STATUS "found thrift")
endif()

include_directories(${Boost_INCLUDE_DIRS})

include_directories(${PROJECT_SOURCE_DIR})

string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
message(STATUS "CXX_FLAGS = " ${CMAKE_CXX_FLAGS} " " ${CMAKE_CXX_FLAGS_${BUILD_TYPE}})

add_subdirectory(muduo/base)
add_subdirectory(muduo/net)

if(MUDUO_BUILD_EXAMPLES)
  add_subdirectory(contrib)
  add_subdirectory(examples)
else()
  if(CARES_INCLUDE_DIR AND CARES_LIBRARY)
    add_subdirectory(examples/cdns)
  endif()
endif()

