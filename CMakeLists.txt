cmake_minimum_required(VERSION 2.5)
project(mymuduo)

#mymuduo最终编译成so动态库设置动态库的路径，放在根目录的文件夹下面
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
#设置调试信息 以及启动c++11语言标准
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")

#定义参与编译的源代码文件
#相当于定义了一个变量名字放在变量名SRC_LIST里面了
# . 表示当前目录所有的源文件
#其实就是把根目录下的源文件名字组合起来放在SRC_LIST变量里面
aux_source_directory(. SRC_LIST)
#编译生成动态库mymuduo
add_library(mymuduo SHARED ${SRC_LIST})