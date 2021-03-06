include prorab.mk

this_name := opros

this_soname := $(shell cat $(prorab_this_dir)soname.txt)

this_srcs := $(call prorab-src-dir,.)

ifeq ($(debug),true)
    this_cxxflags += -DDEBUG
else
    this_cxxflags += -O3
endif

this_cxxflags += -Wall #enable all warnings
this_cxxflags += -Wno-comment #no warnings on nested comments
this_cxxflags += -funsigned-char #the 'char' type is unsigned
this_cxxflags += -Wnon-virtual-dtor #warn if base class has non-virtual destructor
this_cxxflags += -fno-operator-names #do not treat 'and', 'bitand','bitor','compl','not','or','xor' as keywords
this_cxxflags += -Werror #treat warnings as errors
this_cxxflags += -Wfatal-errors # stop on first error encountered
this_cxxflags += -fstrict-aliasing #strict aliasing (in order to comply with the c++ standard more strictly)
this_cxxflags += -std=c++11
this_cxxflags += -g #include debugging symbols
                #-P -E #-Wreorder

this_ldlibs += -lstdc++

ifeq ($(os),linux)
    this_cxxflags += -fPIC # Since we are building shared library, we need Position-Independend Code
else ifeq ($(os),windows)
else ifeq ($(os),macosx)
    this_cxxflags += -stdlib=libc++ #this is needed to be able to use c++11 std lib
endif

this_out_dir := build

$(eval $(prorab-build-lib))
