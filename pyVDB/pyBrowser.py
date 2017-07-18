#!/usr/bin/python

import db_con
import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk
import os
import humanize
from pathlib import Path
import cv2
import sqlite3
from wand.image import Image
from pymediainfo import MediaInfo
    
iconSize = 300
width = 640
height = 480
thumb_time = 10000
default_sort = "size"
default_desc = True
vExts = [ ".mp4", ".mov",".mpg",".mpeg",".wmv",".m4v", ".avi", ".flv" ]
home_dir=os.path.expanduser('~')
app_path = home_dir+"/.video_proj"
if not Path(app_path).is_dir():
    os.mkdir(app_path)
db_file = app_path+"/vdb.sql"
db_file_temp= app_path+"/temp.sql"
temp_icon = app_path+"/temp.png"
directory = "/home/ungermax/mt_test/"
dbCon = db_con.DbConnector(app_path)
vid = dbCon.get_last_vid()

class vid_file(object):
    def __init__(self, fileName="0", size=0, length=0, okflag=0, vdatid =0):
        self.fileName = fileName or "0"
        self.size = size or 0
        self.length = length or 0
        self.okflag = okflag or 0
        self.vdatid  =  vdatid  or 0

    def __repr__(self):
        return "(%s:%i:%i:%i:%i:%i)" % (self.fileName, self.size, self.okflag, self.length, self.vdatid )

class MyWindow(Gtk.Window):
    def __init__(self):
        Gtk.Window.__init__(self,title="PyVDBrowser: " + directory)
        self.set_default_size(width,height)
        box_outer = Gtk.Box(orientation=Gtk.Orientation.VERTICAL,spacing=6)
        self.add(box_outer)
        self.sort_by=default_sort #size, name, length
        self.sort_desc=default_desc #True, False
        browse_button = Gtk.Button(label="...")
        browse_button.connect("clicked",self.browse_clicked)
        fdupe_button = Gtk.Button(label="Find Dupes")
        fdupe_button.connect("clicked",self.fdupe_clicked)
        sort_opt = Gtk.Box()
        sort_list = Gtk.ListStore(str)
        opts = ["size",  "length", "name"]
        for opt in opts:
            sort_list.append([opt])
        sort_label = Gtk.Label("Sort by:")
        self.sort_combo = Gtk.ComboBox.new_with_model(sort_list)
        renderer_text = Gtk.CellRendererText()
        self.sort_combo.pack_start(renderer_text, True)
        self.sort_combo.add_attribute(renderer_text, "text", 0)
        self.sort_combo.set_active(0)
        self.sort_combo.connect("changed",self.on_sort_changed)
        if self.sort_desc:
            stock_icon=Gtk.STOCK_SORT_DESCENDING
        else:
            stock_icon=Gtk.STOCK_SORT_ASCENDING
        self.asc_button = Gtk.Button(None, image=Gtk.Image(stock=stock_icon) )   
        self.asc_button.connect("clicked", self.asc_clicked)
        self.connect("destroy", self.on_delete)
        sort_opt.add(sort_label)
        sort_opt.add(self.sort_combo)
        sort_opt.add(self.asc_button)
        sort_opt.pack_end(browse_button,  False, False, 0 ) 
        box_outer.pack_start(sort_opt, False, True, 0)
        self.scrollWin = Gtk.ScrolledWindow()
        box_outer.pack_start(self.scrollWin, True, True, 0)
        self.populate_icons()
        
        
    def populate_icons(self,  clean=False):
        if clean:
            self.scrollWin.remove(self.fBox)
        self.fBox = Gtk.FlowBox()
        self.fBox.set_sort_func(self.sort_videos)
        for filename in os.listdir(directory):
            fName, fExt = os.path.splitext(filename)
            flExt = fExt.lower()           
            if flExt in vExts:
                video = video_icon(filename)
                self.fBox.add(video)
        self.fBox.invalidate_sort()
        self.scrollWin.add(self.fBox)
        
    def sort_videos(self, videoFile1, videoFile2):
        value=1
        v1=videoFile1.get_child()
        v2=videoFile2.get_child()
        if not self.sort_desc:
            value *= -1
        if self.sort_by == 'size':
            return value*(v1.size - v2.size)
        elif self.sort_by == 'length':
            return value*(v1.length - v2.length)
        else:
            if v1.fileName == v2.fileName:
                 return 0
            list = [ v1.fileName , v2.fileName ]
            slist = sorted(list)
            if slist[0] == v1.fileName:
                 if self.sort_desc:
                      return 1
                 else:
                      return -1
            else:
                 if self.sort_desc:
                      return -1
                 else:
                      return 1

    def add_filters(self, dialog):
        filter_any = Gtk.FileFilter()
        filter_any.set_name("Any files")
        filter_any.add_pattern("*")
        dialog.add_filter(filter_any)

    def browse_clicked(self, widget):
        global directory
        dialog = Gtk.FileChooserDialog("Please choose a folder", self,
            Gtk.FileChooserAction.SELECT_FOLDER,
            (Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL,
             "Select", Gtk.ResponseType.OK))
        dialog.set_default_size(800, 400)
        response = dialog.run()
        if response == Gtk.ResponseType.OK:
            directory = dialog.get_filename()+"/"
            self.populate_icons(True)
        dialog.destroy()
        self.show_all()

    def fdupe_clicked(self,widget):
        global directory
        
        
    def on_sort_changed(self, combo):
        tree_iter = combo.get_active_iter()
        if tree_iter != None:
            model = combo.get_model()
            self.sort_by = model[tree_iter][0]
            self.fBox.invalidate_sort()
            
    def asc_clicked(self, button):
        self.sort_desc=not self.sort_desc
        if self.sort_desc:
            self.asc_button.set_image(Gtk.Image(stock=Gtk.STOCK_SORT_DESCENDING))
        else:
            self.asc_button.set_image(Gtk.Image(stock=Gtk.STOCK_SORT_ASCENDING))
        self.asc_button.set_use_stock(True)
        self.fBox.invalidate_sort()

    def on_delete(self, widget):
        dbCon.save_db_file()
        
class dupe_finder(object):
    def __init__(self,directory):
        videos = []
        for filename in os.listdir(directory):
            fName, fExt = os.path.splitext(filename)
            flExt = fExt.lower()           
            if flExt in vExts:
                videos.append(filename)
        for video in videos:
            video_temp = vid_file(video)
            dbCon.fetch_video(video_temp)
            temp_vid = vid_temp
            dbCon.vur.execute("select * from dat_blobs where vid=?", video_temp.vid)
            temp = (0,0,[0])
            temp = self.cur.fetchone()
            #if temp[3][0] == 0:
                
                
        
        
class video_icon(vid_file, Gtk.EventBox):
    def __init__(self,fileName):
        global vid
        super().__init__()
        Gtk.Image.__init__(self)
        self.fileName =  directory+fileName
        self.size=0.0
        self.length=0.0
        if(dbCon.video_exists(self.fileName)):
            dbCon.fetch_video(self)
            dbCon.cur.execute('select img_dat from dat_blobs where vid=?',(self.vdatid ,))
            imageData = dbCon.cur.fetchone()[0]
            with open(temp_icon, "wb") as output_file:
                output_file.write(imageData)                       
        else:
            vid+=1
            self.vdatid  = vid
            self.size = os.stat(self.fileName).st_size
            self.length = MediaInfo.parse(self.fileName).tracks[0].duration
            self.createIcon(self.fileName)
            dbCon.cur.execute("insert into videos values (?,?,?,?,?)", (self.fileName, self.size, self.length, 1, vid))
            dbCon.con.commit()
        image = Gtk.Image()
        image.set_from_file(temp_icon)
        self.add(image)
        #self.set_tooltip_text("Filename: "+self.fileName+ "\nSize: "+humanize.naturalsize(self.size)+"\nLength: {:0.1f}".format(self.length/1000.0)+"s")
        #self.set_custom(image)
        
    def createIcon(self,filename):
        global vid
        cap=cv2.VideoCapture(filename)
        ret,buf = cap.read()
        while(cap.isOpened() and cap.get(cv2.CAP_PROP_POS_MSEC) < thumb_time):
            ret,buf = cap.read()
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break
            else:
                break
        cv2.imwrite(temp_icon, buf)
        image = Image(filename=temp_icon)
        image.trim(fuzz=5000)
        scale = 1.0
        xScale = 320.0/image.width
        yScale = 200.0/image.height
        if xScale < yScale :
            scale = xScale
        else:
            scale = yScale
        image.resize(int(image.width*scale),  int(image.height*scale))
        image.save(filename=temp_icon)
        dbCon.cur.execute('insert into dat_blobs(vid, img_dat, vdat) values (?,?, "0")', (vid,sqlite3.Binary(open(temp_icon,'rb').read())))
        dbCon.con.commit()
        
    def __repr__(self):
        return repr(self.fileName)

win = MyWindow()
win.connect("delete-event", Gtk.main_quit)
win.show_all()
Gtk.main()
