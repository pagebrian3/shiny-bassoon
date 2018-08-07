#!/usr/bin/python

import zlib
import time
import db_con
import vid_file
import gi
import pickle
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk
import os
import subprocess
import humanize
from pathlib import Path
import sqlite3
import numpy as np
import multiprocessing
import tempfile
from PIL import Image
from pymediainfo import MediaInfo

iconHeight = 180
iconWidth = 320
width = 640
height = 480
thumb_time = 12000
trace_time = "00:00:10"
comp_time = 20
trace_fps= 15.0
border_fps= 0.5
thresh = 1000.0*comp_time*trace_fps
black_thresh = 1.0
fudge = 8
slice_spacing = 60
PROCESSES = 1
FUZZ=5000
default_sort = "size"
default_desc = True
vExts = [ ".mp4", ".mov",".mpg",".mpeg",".wmv",".m4v", ".avi", ".flv" ]
home_dir=os.path.expanduser('~')
app_path = home_dir+"/.video_proj"
if not Path(app_path).is_dir(): os.mkdir(app_path)
db_file = app_path+"/vdb.sql"
db_file_temp= app_path+"/temp.sql"
temp_icon = app_path+"/temp.jpg"
directory = "/home/ungermax/mt_test/"
dbCon = db_con.DbConnector(app_path)
vid = dbCon.get_last_vid()

def calculate_trace(videoFile, vid):
    temp_dir = tempfile.TemporaryDirectory()
    print(videoFile)
    subprocess.run('ffmpeg -y -nostats -loglevel 0 -ss %s -i \"%s\" -r %.2f  %s/border%%05d.bmp' % (trace_time, videoFile,border_fps,temp_dir.name), shell=True)
    icons = []
    for filename in os.listdir(temp_dir.name):
         fName, fExt = os.path.splitext(filename)
         flExt = fExt.lower()           
         if flExt == ".bmp": icons.append(filename)
    icons=sorted(icons)
    last_im_file = Image.open(filename=temp_dir.name+"/"+icons[0])
    last_image = last_im_file.load()
    accum = np.zeros((last_image.size[0],last_image.size[1]), dtype=np.int32)
    icon_counter=0
    #use Image.difference instead.
    for icon in icons[1:]:
        with Image.open(filename=temp_dir.name+"/"+icon) as img_f:
            img = img_f.load()
            print(str(vid)+" "+str(icon_counter))
            for row1,row2,row_accum in zip(img,last_image,accum):
                for pixel1,pixel2,pixel_accum in zip(row1,row2,row_accum):
                    pixel_accum+=pow(int(255*pixel1[0])-int(255*pixel2[0]),2)+pow(int(255*pixel1[1])-int(255*pixel2[1]),2)+pow(int(255*pixel1[2])-int(255*pixel2[2]),2)                           
            last_image=img
        icon_counter+=1
    array_dims = accum.size
    cuts = [0,array_dims[0]-1,0,array_dims[1]-1]
    for i in range(0,array_dims[0]/2):
        if np.sum(accum[i]) > black_thresh*array_dims[1]:
            cuts[0]=i
            break
    for i in reversed(range(array_dims[0]-1,array_dims[0]/2)):
        if np.sum(accum[i]) > black_thresh*array_dims[1]:
            cuts[1]=i
            break
    for i in range(0,array_dims[1]/2):
        if np.sum(accum[:,i]) > black_thresh*array_dims[0]:
            cuts[2]=i
            break
    for i in reversed(range(array_dims[1]-1,array_dims[1]/2)):
        if np.sum(accum[:,i]) > black_thresh*array_dims[0]:
            cuts[3]=i
            break
    print(cuts)
    subprocess.run('ffmpeg -y -nostats -loglevel 0 -i \"%s\" -r %.2f -s 2x2  %s/out%%05d.bmp' % (videoFile,trace_fps,temp_dir.name), shell=True)
    icons = []
    trace = []
    for filename in os.listdir(temp_dir.name):
         fName, fExt = os.path.splitext(filename)
         flExt = fExt.lower()           
         if flExt == ".bmp": icons.append(filename)
    icons=sorted(icons)
    for icon in icons:
        with Image.open(filename=temp_dir.name+"/"+icon) as img_f:
            img = img_f.load()
            for row in img:
                for pixel in row:
                    trace.append(int(255*pixel[0]))
                    trace.append(int(255*pixel[1]))
                    trace.append(int(255*pixel[2]))
    return (vid,trace)

class MyWindow(Gtk.Window):
    def __init__(self):
        Gtk.Window.__init__(self,title="PyVDBrowser: " + directory)
        time1 = time.perf_counter()
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
        for opt in opts: sort_list.append([opt])
        sort_label = Gtk.Label("Sort by:")
        self.sort_combo = Gtk.ComboBox.new_with_model(sort_list)
        renderer_text = Gtk.CellRendererText()
        self.sort_combo.pack_start(renderer_text, True)
        self.sort_combo.add_attribute(renderer_text, "text", 0)
        self.sort_combo.set_active(0)
        self.sort_combo.connect("changed",self.on_sort_changed)
        if self.sort_desc: stock_icon=Gtk.STOCK_SORT_DESCENDING
        else: stock_icon=Gtk.STOCK_SORT_ASCENDING
        self.asc_button = Gtk.Button(None, image=Gtk.Image(stock=stock_icon) )
        self.asc_button.connect("clicked", self.asc_clicked)
        self.connect("destroy", self.on_delete)
        sort_opt.add(sort_label)
        sort_opt.add(self.sort_combo)
        sort_opt.add(self.asc_button)
        sort_opt.pack_end(fdupe_button,  False,  False,  0)
        sort_opt.pack_end(browse_button,  False, False, 0 ) 
        box_outer.pack_start(sort_opt, False, True, 0)
        self.scrollWin = Gtk.ScrolledWindow()
        box_outer.pack_start(self.scrollWin, True, True, 0)
        self.populate_icons()
        print("TIME1: "+str(time.perf_counter()-time1))
        
    def populate_icons(self,  clean=False):
        if clean: self.scrollWin.remove(self.fBox)
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
        if not self.sort_desc: value *= -1
        if self.sort_by == 'size': return value*(v1.size - v2.size)
        elif self.sort_by == 'length': return value*(v1.length - v2.length)
        else:
            if v1.fileName == v2.fileName: return 0
            list = [ v1.fileName , v2.fileName ]
            slist = sorted(list)
            if slist[0] == v1.fileName:
                 if self.sort_desc: return 1
                 else: return -1
            else:
                 if self.sort_desc: return -1
                 else: return 1

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
        dupe_finder()
        
    def on_sort_changed(self, combo):
        tree_iter = combo.get_active_iter()
        if tree_iter != None:
            model = combo.get_model()
            self.sort_by = model[tree_iter][0]
            self.fBox.invalidate_sort()
            
    def asc_clicked(self, button):
        self.sort_desc=not self.sort_desc
        if self.sort_desc: self.asc_button.set_image(Gtk.Image(stock=Gtk.STOCK_SORT_DESCENDING))
        else: self.asc_button.set_image(Gtk.Image(stock=Gtk.STOCK_SORT_ASCENDING))
        self.asc_button.set_use_stock(True)
        self.fBox.invalidate_sort()

    def on_delete(self, widget):
        dbCon.save_db_file()

class dupe_finder(object):
    def __init__(self):
        with multiprocessing.Pool(PROCESSES) as pool:
            videos = []
            vids = []
            time1 = time.perf_counter()
            for filename in os.listdir(directory):
                fName, fExt = os.path.splitext(filename)
                flExt = fExt.lower()           
                if flExt in vExts:
                    full_path = directory+filename
                    vid_obj= dbCon.fetch_video(full_path)
                    video_id = vid_obj.vdatid
                    vids.append((video_id,))
                    if not dbCon.trace_exists(full_path):
                        #print("Appending: "+full_path)
                        videos.append((full_path,video_id))
            result_array=[pool.apply_async(calculate_trace, v) for v in videos]
            traces = []
            for res in result_array: traces.append(res.get())
            for trace in traces:
                #print(trace)
                dbCon.cur.execute("update dat_blobs set vdat=? where vid=?",(sqlite3.Binary(zlib.compress(pickle.dumps(trace[1]))), trace[0]))
            dbCon.con.commit()
            dbCon.cur.execute('select * from results')
            results = dbCon.cur.fetchall()
            result_map = dict()
            for a in results: result_map.update({(a[0],a[1]):a[2]})
            first_pos=0
            #loop over files  TODO-make this parallel when we have larger sample
            for i in vids[:-1]:
                dbCon.cur.execute('select vdat from dat_blobs where vid=?', i)
                vdat1 = np.array(pickle.loads(zlib.decompress(dbCon.cur.fetchone()[0])))
                #loop over files after i
                for j in vids[first_pos+1:]:
                    #print(str(i[0])+" "+str(j[0]))
                    if (i[0],j[0]) in result_map: continue
                    match=False
                    dbCon.cur.execute('select vdat from dat_blobs where vid=?', j)
                    vdat2 = np.array(pickle.loads(zlib.decompress(dbCon.cur.fetchone()[0])))
                    #loop over slices
                    for t_s in range(0,int(len(vdat1)-12*trace_fps*comp_time), int(12*trace_fps*slice_spacing)):
                        if match: break
                        #starting offset for 2nd trace -this is the loop for the indiviual tests
                        for t_x in range(0,int(len(vdat2)-12*trace_fps*comp_time),12):
                            if match: break
                            accum=np.zeros(12)
                            #offset loop
                            for t_o in range(0,int(12*comp_time*trace_fps),12):
                                counter = 0
                                for a in accum:
                                    if a > thresh: counter+=1
                                if counter != 0: break
                                #pixel/color loop
                                for t_d in range(0,12): 
                                    value = pow(int(vdat1[t_s+t_o+t_d])-int(vdat2[t_x+t_o+t_d]),2)-fudge**2
                                    if value < 0: value = 0
                                    accum[t_d]+=value
                            counter = 0
                            for a in accum:
                                if a < thresh: counter+=1
                            if counter == 12: match=True
                            if match : print("ACCUM "+str(i[0])+" "+str(j[0])+" "+str(t_o)+" slice "+str(t_s)+" 2nd offset "+str(t_x)+" "+str(accum))                
                    result = 0
                    if match: result = 1
                    dbCon.cur.execute('insert into results values (?,?,?)',(i[0], j[0],result))       
                first_pos+=1
            dbCon.con.commit()
            print("TIME2: "+str(time.perf_counter()-time1))
                
class video_icon(vid_file.vid_file, Gtk.EventBox):
    def __init__(self,fileName):
        global vid
        super().__init__()
        Gtk.Image.__init__(self)
        self.fileName =  directory+fileName
        self.size=0.0
        self.length=0.0      
        if(dbCon.video_exists(self.fileName)):
            vid_temp = dbCon.fetch_video(self.fileName)
            self.vdatid = vid_temp.vdatid
            self.size = vid_temp.size
            self.length = vid_temp.length
            dbCon.cur.execute('select img_dat from dat_blobs where vid=?',(self.vdatid ,))
            imageData = dbCon.cur.fetchone()[0]
            with open(temp_icon, "wb") as output_file:
                output_file.write(imageData)                       
        else:
            vid+=1
            self.vdatid  = vid
            self.size = os.stat(self.fileName).st_size
            self.length = MediaInfo.parse(self.fileName).tracks[0].duration
            thumb_t = thumb_time
            if self.length < thumb_time: thumb_t = self.length/2.0
            subprocess.run('ffmpeg -y -nostats -loglevel 0 -ss 00:00:%i.00 -i \"%s\" -vframes 1 -f image2pipe -vcodec png - | convert png:- -fuzz %i -trim -thumbnail %ix%i %s' % (thumb_t/1000, self.fileName, FUZZ,iconWidth,iconHeight,temp_icon), shell=True)
            dbCon.cur.execute('insert into dat_blobs(vid, img_dat, vdat) values (?,?, "0")', (vid,sqlite3.Binary(open(temp_icon,'rb').read())))
            dbCon.cur.execute("insert into videos values (?,?,?,?,?)", (self.fileName, self.size, self.length, 1, self.vdatid))
            dbCon.con.commit()
        image = Gtk.Image()
        image.set_from_file(temp_icon)
        self.add(image)
        self.set_tooltip_text("Filename: "+self.fileName+ "\nSize: "+humanize.naturalsize(self.size)+"\nLength: {:0.1f}".format(self.length/1000.0)+"s")
        
    def __repr__(self):
        return repr(self.fileName)

win = MyWindow()
win.connect("delete-event", Gtk.main_quit)
win.show_all()
Gtk.main()
