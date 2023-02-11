cmake_minimum_required(VERSION 3.4)

project(glad)

add_library(glad OBJECT
    include/glad/glad.h
    include/KHR/khrplatform.h
    src/glad.c
)

target_include_directories(glad PUBLIC
    include
)
