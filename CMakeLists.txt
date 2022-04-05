cmake_minimum_required(VERSION 3.13)
project(lishp C)

set(CMAKE_C_STANDARD 11)

add_executable(lishp main.c common.c tokenizer.c reader.c printer.c eval.c builtins.c io.c shell.c)
