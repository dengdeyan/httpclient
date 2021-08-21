cmake_minimum_required(VERSION 3.13)
project(httpclient C)

add_library(httpclient SHARED
        httpclient.h
        httpclient.c
	stringx.h 
	stringx.c 
	urlparser.h 
	urlparser.c )
if(WIN32)
    target_link_libraries(httpclient ws2_32)
endif()
