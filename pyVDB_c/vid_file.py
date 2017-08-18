#!/usr/bin/python

class vid_file(object):
    def __init__(self, fileName="0", size=0, length=0, okflag=0, vdatid =0):
        self.fileName = fileName or "0"
        self.size = size or 0
        self.length = length or 0
        self.okflag = okflag or 0
        self.vdatid  =  vdatid  or 0

    def __repr__(self):
        return "(%s:%i:%i:%i:%i:%i)" % (self.fileName, self.size, self.okflag, self.length, self.vdatid )
