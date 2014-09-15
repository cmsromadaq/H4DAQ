CXX		=g++
LD		=g++
CXXFLAGS	=-O2 -ggdb 
LDFLAGS		=-lz -lm -lzmq
SOFLAGS		=-fPIC -shared
SHELL		=bash

Packages	=controller testDataTypeServer testDataTypeClient testDataType
Objects		=Daemon EventBuilder Handler Logger Profiler  Configurator ControlManager ConnectionManager Utility HwManager AsyncUtils
LibName		=H4DAQ

###
SrcSuf        = cpp
HeadSuf       = hpp
ObjSuf        = o
DepSuf        = d
DllSuf        = so
StatSuf       = a
### ----- OPTIONS ABOVE ----- ####

include MakeFile.ROOT

BASEDIR=$(shell pwd)
BINDIR=$(BASEDIR)/bin
SRCDIR = $(BASEDIR)/src
HDIR = $(BASEDIR)/interface

BINOBJ	=$(patsubst %,$(BINDIR)/%.$(ObjSuf),$(Objects) )
SRCFILES=$(patsubst %,$(SRCDIR)/%.$(SrcSuf),$(Objects) )
HFILES	=$(patsubst %,$(HDIR)/%.$(HeadSuf),$(Objects) )
StatLib		=$(BINDIR)/H4DAQ.$(StatSuf)
SoLib		=$(BINDIR)/H4DAQ.$(DllSuf)

.PRECIOUS:*.ObjSuf *.DepSuf *.DllSuf


############### EXPLICIT RULES ###############
.PHONY: all
all: info $(Packages) | $(BINDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

info:
	@echo "--------------------------"
	@echo "Compile on $(shell hostname)"
	@echo "Packages are: $(Packages)"
	@echo "Objects are: $(Objects)"
	@echo "--------------------------"
	@echo "DEBUG:"

$(StatLib): $(BINOBJ)
	ar rcs $@ $(BINOBJ)
.PHONY: soLib
soLib: $(SoLib)

$(SoLib): $(StatLib)
	$(LD) $(LDFLAGS) $(SOFLAGS) -o $@ $^

.PHONY: $(Packages) 
$(Packages): % : $(BINDIR)/% | $(BINDIR)
	@echo $(call InfoLine , $@ )

#$(BINDIR)/$(Packages): $(BINDIR)/% : $(BASEDIR)/test/%.$(SrcSuf) $(StatLib) | $(BINDIR)
$(addprefix $(BINDIR)/,$(Packages)): $(BINDIR)/% : $(BASEDIR)/test/%.$(SrcSuf) $(StatLib) | $(BINDIR)
	@echo $(call InfoLine , $@ )
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(StatLib) $<

#make this function of $(Packages)
#.PHONY: controller
#controller: bin/controller
#bin/controller: test/controller.cpp $(BINOBJ) $(StatLib)
#	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(BINOBJ) $<


.PHONY: clean
clean:
	-rm -v bin/controller
	-rm -v bin/*.$(ObjSuf)
	-rm -v bin/*.$(DllSef)


############### IMPLICIT RULES ###############


#InfoLine = \033[01\;31m compiling $(1) \033[00m
InfoLine = compiling $(1)

#.o
%.$(ObjSuf): $(BINDIR)/%.$(ObjSuf)

#.o
$(BINDIR)/%.$(ObjSuf): $(SRCDIR)/%.$(SrcSuf) $(HDIR)/%.$(HeadSuf)
	@echo $(call InfoLine , $@ )
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -c -o $@ $(SRCDIR)/$*.$(SrcSuf)

#.d
$(BINDIR)/%.$(DepSuf): $(SRCDIR)/%.$(SrcSuf) $(HDIR)/%.$(HeadSuf)
	@echo $(call InfoLine , $@ )
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -M -o $@ $(SRCDIR)/$*.$(SrcSuf)


