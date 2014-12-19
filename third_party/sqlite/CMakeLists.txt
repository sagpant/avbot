cmake_minimum_required(VERSION 2.6)
project(sqlite3)

include_directories(include)

add_library(sqlite3 STATIC lib/sqlite3.c)
