cmake_minimum_required(VERSION 3.25) # NÊN DÙNG VERSION 3.25+ CHO C23

# Tên dự án và ngôn ngữ C
project(TheProject C)

# 1. Cấu hình tiêu chuẩn C23
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 2. Hỗ trợ Threads (pthread cho Linux/macOS)
find_package(Threads REQUIRED)

# 3. Tự động quét toàn bộ file .c và .h trong thư mục src
file(GLOB_RECURSE SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/TheProject-C/src/*.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/TheProject-C/src/*.h"
)

# 4. Tạo file thực thi từ các file nguồn vừa quét được
add_executable(${PROJECT_NAME} ${SOURCES}
        TheProject-C/src/thread/threads/pool/ThreadPool.c
        TheProject-C/src/thread/threads/pool/PrivatizedThreadPool.h
        TheProject-C/src/thread/threads/pool/PrivatizedThrreadPool.c)

# 5. Khai báo các thư mục include
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/thread/threads/pool
)

# 6. Liên kết thư viện luồng (Threads)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)