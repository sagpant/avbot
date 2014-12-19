
include(CheckCXXCompilerFlag)

if (NOT MSVC)

if(NOT CMAKE_CXX_COMPILER MATCHES "icc")

CHECK_CXX_COMPILER_FLAG(-std=gnu++11 LIBCXX_HAS_STDCXX11_FLAG)

if(LIBCXX_HAS_STDCXX11_FLAG)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
else()

	CHECK_CXX_COMPILER_FLAG(-std=gnu++0x LIBCXX_HAS_STDCXX0X_FLAG)

	if(LIBCXX_HAS_STDCXX0X_FLAG)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x")
	else()
		message(FATAL "need at least gcc 4.4.7 or clang 3.2")
	endif()

endif()

endif() # if(NOT CMAKE_CXX_COMPILER MATCHES "icc")
endif(NOT MSVC)
