CXX=g++
LD=g++
CXXFLAGS=-O2 -ggdb 
LDFLAGS=-lz -lmath -lzqm
SOFLAGS=-fPIC -shared
SHELL=bash

Packages=controller

###
SrcSuf        = cpp
HeadSuf       = hpp
ObjSuf        = o
DepSuf        = d
DllSuf        = so
### ----- OPTIONS ABOVE ----- ####

BASEDIR=$(shell pwd)
BINDIR=$(BASEDIR)/bin
SRCDIR = $(BASEDIR)/src
HDIR = $(BASEDIR)/interface


############### EXPLICIT RULES ###############
dirs:
	mkdir -p $(BINDIR)


.PHONY: all
all: controller
.PHONY: controller
controller: bin/controller

.PHONY: clean
clean:
	-rm -v bin/controller
	-rm -v bin/*.$(ObjSuf)
	-rm -v bin/*.$(DllSef)

bin/controller: test/controller.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

############### IMPLICIT RULES ###############


#InfoLine = \033[01\;31m compiling $(1) \033[00m
InfoLine = compiling $(1)

#.o
$(BINDIR)/%.$(ObjSuf): $(SRCDIR)/%.$(SrcSuf) $(HEADDIR)/%.$(HeadSuf)
	@echo $(call InfoLine , $@ )
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -c -o $@ $(SRCDIR)/$*.$(SrcSuf)

#.d
$(BINDIR)/%.$(DepSuf): $(SRCDIR)/%.$(SrcSuf) $(HEADDIR)/%.$(HeadSuf)
        @echo $(call InfoLine , $@ )
        $(CXX) $(CXXFLAGS) $(LDFLAGS) -M -o $@ $(SRCDIR)/$*.$(SrcSuf)


