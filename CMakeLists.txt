cmake_minimum_required(VERSION 3.25)
project(ProjectC C)

# Yêu cầu chuẩn C23 và tắt các extension của trình biên dịch
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(Threads REQUIRED)

# Thêm tất cả các thư mục chứa header (.h) vào đường dẫn include của dự án
include_directories(
        include
        concurrent/pool
        landform
        lib/matrix
)

# Tự động tìm tất cả các file nguồn (.c) trong thư mục src và các thư mục con
file(GLOB_RECURSE SOURCES
        "*.c"
        "concurrent/**/*.c"
        "landform/**/*.c"
        "lib/**/*.c"
)

# (Tùy chọn) Tìm cả file header (.h) để chúng hiển thị gọn gàng trong các IDE như CLion/VS Code
file(GLOB_RECURSE HEADERS
        "*.h"
        "concurrent/**/*.h"
        "landform/**/*.h"
        "lib/**/*.h"
)

# Tạo file thực thi từ các file nguồn (CMake tự liên kết các file .h tương ứng)
add_executable(ProjectC ${SOURCES} ${HEADERS})