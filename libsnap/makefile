#Set this to @ to keep the makefile quiet
SILENCE = @

#---- Outputs ----#
COMPONENT_NAME = snap
CPPUTEST_LIB_DIR = ../lib/

#--- Inputs ----#
PROJECT_HOME_DIR = .
CPPUTEST_HOME = ../CppUTest

USER_LIBS = ../lib/libmocks.a ../lib/libcommon.a

CPP_PLATFORM = Gcc

CPPUTEST_CPPFLAGS += -fno-common
CPPUTEST_WARNINGFLAGS += -Wall 
CPPUTEST_WARNINGFLAGS += -Werror 
CPPUTEST_WARNINGFLAGS += -Wswitch-default 
CPPUTEST_WARNINGFLAGS += -Wswitch-enum
CPPUTEST_WARNINGFLAGS += -Wno-unused-parameter
CPPUTEST_WARNINGFLAGS += -Wno-overlength-strings
CPPUTEST_CFLAGS += -std=gnu99
CPPUTEST_CFLAGS += -Wextra 
CPPUTEST_CFLAGS += -Wstrict-prototypes
CPPUTEST_CFLAGS += -DCODE_UNDER_TEST

SRC_DIRS = \
	src\


TEST_SRC_DIRS = \
	tests \
	
INCLUDE_DIRS =\
  $(CPPUTEST_HOME)/include/ \
  ../include/               \
  tests/                    \

include $(CPPUTEST_HOME)/build/MakefileWorker.mk
