# Copyright (C) 2012  Adam Green (https://github.com/adamgreen)
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# Directories to be built
DIRS=CppUTest libmocks libcommon libsnap libcrackle snap crackle
DIRSCLEAN = $(addsuffix .clean,$(DIRS))

all: $(DIRS)

clean: $(DIRSCLEAN)

$(DIRS):
	@echo Building $@
	@ $(MAKE) -C $@ all

$(DIRSCLEAN): %.clean:
	@echo Cleaning $*
	@ $(MAKE) -C $*  clean

.PHONY: all clean $(DIRS) $(DIRSCLEAN)
