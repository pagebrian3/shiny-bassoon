#include "VBrowser.h"
#include "MediaInfo/MediaInfo.h"
#define THUMB_TIME  12000
#define ICON_WIDTH 320
#define ICON_HEIGHT 180
#define FUZZ 5000
#define FUDGE 8

std::set<std::string> extensions{".mp4",".wmv",".mov",".rm",".m4v",".flv",".avi",".qt",".mpg",".mpeg",".mpv",".3gp"};

VBrowser::VBrowser() {
  Gtk::VBox * box_outer = new GTK::VBox(false, 6);
  this->add(box_outer);
  sort_by=default_sort; //size, name, length
  sort_desc=default_desc;  //true, false
  browse_button = new Gtk::Button("...");
  browse_button->signal_clicked().connect(sigc::ptr_fun(&browse_clicked));
  fdupe_button = new Gtk::Button("Find Dupes");
  fdupe_button->signal_clicked().connect(sigc::ptr_fun(&fdupe_clicked));
  GtkBox * sort_opt = new Gtk::Box();
  GtkLabel * sort_label = new Gtk::Label("Sort by:");
  sort_combo = new Gtk::ComboBoxText();
  sort_combo->append("size");
  sort_combo->append("length");
  sort_combo->append("name");
  sort_combo->set_active(0);
  sort_combo->on_changed().connect(sigc::ptr_fun(&on_sort_changed));
  GtkStockItem stock_icon = GTK_STOCK_SORT_ASCENDING);
  if (sort_desc) stock_icon=GTK_STOCK_SORT_DESCENDING
  Gtk::Button * asc_button = new Gtk::Button();
  asc_button->set_image(asc_button,Gtk::Image(stock_icon));
  asc_button->signal_clicked().connect(sigc::ptr_fun(&asc_clicked));
  sort_opt->add(sort_label);
  sort_opt->add(sort_combo);
  sort_opt->add(asc_button);
  sort_opt->pack_end(fdupe_button,  false,  false,  0);
  sort_opt->pack_end(browse_button,  false, false, 0 ); 
  box_outer->pack_start(sort_opt, false, true, 0);
  fScrollWin = new Gtk::ScrolledWindow(NULL,NULL);
  box_outer->pack_start(fScrollWin, true, true, 0);
  populate_icons();
}

void VBrowser::populate_icons(bool clean=false) {
  if(clean) fScrollWin->remove(fFBox);
  fFBox = new Gtk::FlowBox();
  fFBox->set_sort_func(sort_videos,NULL,NULL);
  std::vector<bfs::directory_entry x> video_list;
  for (bfs::directory_entry & x : bfs::recursive_directory_iterator(path))
    {
      auto extension = x.path().extension().generic_string();
      std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
      if(extensions.count(extension)) fBox->add(VideoIcon(x.path().str(),dbCon));     
    }
  fFBox->invalidate_sort();
  fScrollWin->add(fFbox);
}

int VBrowser::sort_videos(VideoIcon videoFile1, VideoIcon videoFile2) {
  int value=1;
  vid_file v1=videoFile1.get_child().get_vid_file();
  vid_file v2=videoFile2.get_child().get_vid_file();
  if(!self.sort_desc) value *= -1;
  if(self.sort_by == 'size') return value*(v1.size - v2.size);
  else if(sort_by == 'length') return value*(v1.length - v2.length);
  else return value*std::compare(v1.fileName,v2.fileName);   
}

void VBrowser::browse_clicked() {
  Gtk::FileChooserDialog dialog("Please choose a folder", self,
				 Gtk.FileChooserAction.SELECT_FOLDER,
				 (Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL,
				  "Select", Gtk.ResponseType.OK));
  dialog.set_default_size(800, 400);
  Gtk::FileFilter filter_any;
        filter_any.set_name("Any files");
        filter_any.add_pattern("*");
        dialog.add_filter(filter_any);
  auto response = dialog.run();
  if (response == Gtk::ResponseType.OK){
    path  = dialog.get_filename()+"/";
    self.populate_icons(true);
  }
  dialog.destroy();
  this->show_all();
}

void VBrowser::on_delete() {
  dbCon->save_db_file();
}

void VBrowser::fdupe_clicked(){
        with multiprocessing.Pool(PROCESSES) as pool:
            videos = []
            vids = []
            for filename in os.listdir(directory):
                fName, fExt = os.path.splitext(filename)
                flExt = fExt.lower()           
                if flExt in vExts:
                    full_path = directory+filename
                    vid_obj= dbCon.fetch_video(full_path)
                    video_id = vid_obj.vdatid
                    vids.append((video_id,))
                    if not dbCon.trace_exists(full_path):
                        videos.append((vid_obj,))
            result_array=[pool.apply_async(calculate_trace, v) for v in videos]
            traces = []
            for res in result_array: traces.append(res.get())
            for trace in traces:
  //print(trace)
                dbCon.cur.execute("update dat_blobs set vdat=? where vid=?",(sqlite3.Binary(zlib.compress(pickle.dumps(trace[1]))), trace[0]))
            dbCon.con.commit()
			std::map<std::pair<int,int>,int> result_map;
	dbCon->fetch_results(result_map);
			
	for(auto a : results) result_map[std::make_pair(a[0],a[1])] = a[2];
            first_pos=0
            time1 = time.perf_counter()
		    //loop over files  TODO-make this parallel when we have larger sample
		    for(int i = 0; i +1 < vids.size(); i++) {
                dbCon.cur.execute('select vdat from dat_blobs where vid=?', i)
                vdat1 = np.array(pickle.loads(zlib.decompress(dbCon.cur.fetchone()[0])))
		    //loop over files after i
                for(int j = i+1; j < vids.size(); j++) {
  //print(str(i[0])+" "+str(j[0]))
                    if (i[0],j[0]) in result_map: continue
                    match=False
                    dbCon.cur.execute('select vdat from dat_blobs where vid=?', j)
                    vdat2 = np.array(pickle.loads(zlib.decompress(dbCon.cur.fetchone()[0])))
				     //loop over slices
				     for(int t_s =0; t_s < len(vdat1)-12*trace_fps*comp_time; t_s+= 12*trace_fps*slice_spacing){
  if(match) break;
  //starting offset for 2nd trace -this is the loop for the indiviual tests
				    for(int t_x = 0; t_x < len(vdat2)-12*trace_fps*comp_time; t_x+=12){
  if(match) break;
					std::vector<int> accum(12);
					//offset loop
					for(int t_o = 0; t_o < 12*comp_time*trace_fps; t_o+=12){
                                      counter = 0;
				      for(a in accum) if (a > thresh) counter+=1;
					     if(counter != 0) break;
	//pixel/color loop
						for (unit t_d = 0; t_d < 12; td++) {
						  value = pow(int(vdat1[t_s+t_o+t_d])-int(vdat2[t_x+t_o+t_d]),2)-pow(fudge,2);
						  if(value < 0) value = 0;
						  accum[t_d]+=value;
						}
					}
					       counter = 0;
					       for(a in accum)  if(a < thresh) counter+=1;
				       if(counter == 12) match=true;
  if(match) std::cout << ("ACCUM "+str(i[0])+" "+str(j[0])+" "+str(t_o)+" slice "+str(t_s)+" 2nd offset "+str(t_x)+" "+str(accum))  <<std::endl;
				    }
				     }
				       result = 0;
				       if match: result = 1;
				       dbCon.cur.execute('insert into results values (?,?,?)',(i[0], j[0],result)) ;      
				       first_pos+=1;
				       dbCon.con.commit();
				       print("TIME2: "+str(time.perf_counter()-time1));
				       }

VideoIcon::VideoIcon(char *fileName, DbConnector * dbCon){
  char * img_dat;
  if(dbCon->video_exists(fileName)) {
    fVidFile = dbCon->fetch_video(fileName);
    img_dat = dbCon->fetch_icon(fVidFile.vdatid);
    }
  else {
    fVidFile.fileName=fileName;
    fVidFile.size = bfs::file_size(fileName);
    MediaInfoLib::MediaInfo MI;
    MI.Open(__T(fileName));
    double length = std::atod(MI.Get(Stream_General, 0 ,__T("Length"), Info_Text, Info_Name).c_str()); //Might be duration instead of length
    fVidFile.length = length;
    double thumb_t = THUMB_TIME;
    if(length < THUMB_TIME) thumb_t = length/2.0;
    std::system((boost::format("ffmpeg -y -nostats -loglevel 0 -ss 00:00:%i.00 -i \"%s\" -vframes 1 -f image2pipe -vcodec png - | convert png:- -fuzz %i -trim -thumbnail %ix%i %s") % thumb_t/1000 % fileName % FUZZ % ICON_WIDTH % ICON_HEIGHT % temp_icon ).str().c_str());
    dbCon->save_icon(vid);
  }
  GTK::Image image(dbCon->temp_icon_file());
  this->add(image);
  this->set_tooltip_text("Filename: "+fileName+ "\nSize: "+fVidFile.size/1024+"kB\nLength: " +fVidFile.length/1000.0+"s");
};

VideoIcon::get_vid_file() {
  return fVidFile;
};

