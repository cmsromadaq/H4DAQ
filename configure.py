#/usr/bin/env python

import os,sys,array

from optparse import OptionParser
from subprocess import call

usage="usage: %prog [options] "
parser=OptionParser(usage)

parser.add_option("","--noroot",action="store_true",help="NO ROOT",default=False)
parser.add_option("","--nocaen",action="store_true",help="NO CAEN",default=False)
parser.add_option("","--noxml" ,action="store_true",help="NO XML",default=False)
parser.add_option("","--nozmq" ,action="store_true",help="NO ZMQ",default=False)

(opts,argv)=parser.parse_args();

cmd=["cp","-v" ,"make/Makefile.in","Makefile"]
call(cmd)

if opts.noroot:
	print "-> NO ROOT"
	cmd=["sed","-i","s:Makefile\.ROOT:Makefile.NOROOT:","Makefile"]
	call(cmd)
if opts.noxml:
	print "-> NO XML"
	cmd=["sed","-i","s:Makefile\.XML:Makefile.NOXML:","Makefile"]
	call(cmd)
if opts.nocaen:
	print "-> NO CAEN"
	cmd=["sed","-i","s:Makefile\.CAEN:Makefile.NOCAEN:","Makefile"]
	call(cmd)
if opts.nozmq:
	print "-> NO ZMQ"
	cmd=["sed","-i","s:Makefile\.ZMQ:Makefile.NOZMQ:","Makefile"]
	call(cmd)

print "DONE"


