#!/usr/bin/python
import sqlite3
import vid_file
import io
import os
from pathlib import Path

class DbConnector(object):
    def __init__(self,db_path):
        self.newFile = True
        self.db_path=db_path
        self.db_file=db_path+"/vdb.sql"
        if Path(self.db_file).is_file():
            self.newFile=False
        # Read database to tempfile
        tempfile = io.StringIO()
        self.con = sqlite3.connect(":memory:")
        self.cur = self.con.cursor()
        db_file_temp = self.db_path+"/temp.sql"
        if self.newFile:
            conTemp = sqlite3.connect(db_file_temp)
            conTemp.execute("create table results(v1id integer, v2id integer, result integer, regions text)")
            conTemp.execute("create table dat_blobs(vid integer primary key, img_dat blob, vdat blob)")
            conTemp.execute("create table videos(path text,length real, size real, okflag integer, vdatid integer)")#, foreign key(vdatid) references dat_blobs(vid)")
            conTemp.commit()
            for line in conTemp.iterdump():
                tempfile.write('%s\n' % line)
            tempfile.seek(0)
            self.cur.executescript(tempfile.read())
            os.remove(db_file_temp)
        else:
            f=open(self.db_file,'r')
            str = f.read()
            self.cur.executescript(str)
            os.remove(self.db_file)
        self.con.commit()
        self.con.row_factory = sqlite3.Row
        
    def save_db_file(self):
        with open(self.db_file, 'w') as f:
            for line in self.con.iterdump():
                f.write('%s\n' % line)  

    def video_exists(self,filename):
        self.cur.execute("select name from sqlite_master where type = 'table'")
        self.cur.execute("select count(1) from videos where path=?",(filename,))
        return self.cur.fetchone()[0]

    def trace_exists(self,filename):
        id = self.fetch_video(filename).vdatid
        self.cur.execute("select vdat from dat_blobs where vid=?", (id,))
        result = self.cur.fetchone()
        if result[0] == "0":
            return False
        return True
        
    def fetch_video(self,filename):
        temp = ("0",0,0,0,0)
        self.cur.execute("select * from videos where path=?", (filename,))
        temp = self.cur.fetchone()
        vid_obj = vid_file.vid_file()
        vid_obj.fileName = temp[0]
        vid_obj.size     = temp[1]
        vid_obj.length   = temp[2]
        vid_obj.okflag   = temp[3]
        vid_obj.vdatid   = temp[4]
        return vid_obj

    def get_last_vid(self):
        if self.newFile:
            return 0
        else:
            self.cur.execute("SELECT MAX(vid) from dat_blobs")
            return self.cur.fetchone()[0]
