include prorab.mk

this_name := tests

this_cxxflags += -std=c++11
this_cxxflags += -Wall
this_cxxflags += -fstrict-aliasing
this_cxxflags += -g

ifeq ($(debug),true)
    this_cxxflags += -DDEBUG
else
    this_cxxflags += -O3
endif

this_srcs += main.cpp tests.cpp ../helpers/queue.cpp

this_ldlibs += -lopros -lstdc++

ifeq ($(os),macosx)
    this_cxxflags += -stdlib=libc++ # this is needed to be able to use c++11 std lib
    this_ldlibs += -lc++
else ifeq ($(os),windows)
else
    this_cxxflags += -fPIC
    this_ldlibs += -lpthread
endif

this_ldflags += -L$(d)../../src/build

this_no_install := true

$(eval $(prorab-build-app))

# add dependency on libopros.so
$(prorab_this_name): $(abspath $(d)../../src/build/libopros$(dot_so))

include $(d)../test_target.mk


#include makefile for building opros
$(eval $(call prorab-include, ../../src/makefile))
