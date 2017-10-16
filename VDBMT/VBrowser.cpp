#include "VBrowser.h"
#include <wand/MagickWand.h>
#include <boost/process.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <set>

#define WIN_WIDTH 800
#define WIN_HEIGHT 600
#define FUDGE 10
#define TRACE_TIME 10.0
#define TRACE_FPS 25.0
#define BORDER_FRAMES 20.0
#define CUT_THRESH 1000.0
#define COMP_TIME 10.0
#define SLICE_SPACING 60
#define THRESH 2000.0*COMP_TIME*TRACE_FPS
#define EXTENSION ".png"
#define HOME_PATH "/home/ungermax/.video_proj/"
#define DEFAULT_PATH "/home/ungermax/mt_test/"
#define DEFAULT_SORT "size"
#define ICON_SIZE 6
#define NUM_THREADS 1
#define DEFAULT_DESC true

std::set<std::string> extensions{".mp4",".wmv",".mov",".rm",".m4v",".flv",".avi",".qt",".mpg",".mpeg",".mpv",".3gp"};

VBrowser::VBrowser() {
  MagickWandGenesis();
  TPool = new cxxpool::thread_pool(NUM_THREADS);
  this->set_default_size(WIN_WIDTH, WIN_HEIGHT);
  dbCon = new DbConnector();
  Gtk::VBox * box_outer = new Gtk::VBox(false, ICON_SIZE);
  this->add(*box_outer);
  path = DEFAULT_PATH;
  sort_by=DEFAULT_SORT; //size, name, length
  sort_desc=DEFAULT_DESC;  //true, false
  browse_button = new Gtk::Button("...");
  browse_button->signal_clicked().connect(sigc::mem_fun(*this,&VBrowser::browse_clicked));
  fdupe_button = new Gtk::Button("Find Dupes");
  fdupe_button->signal_clicked().connect(sigc::mem_fun(*this,&VBrowser::fdupe_clicked));
  Gtk::Box * sort_opt = new Gtk::Box();
  Gtk::Label * sort_label = new Gtk::Label("Sort by:");
  sort_combo = new Gtk::ComboBoxText();
  sort_combo->append("size");
  sort_combo->append("length");
  sort_combo->append("name");
  sort_combo->set_active(0);
  sort_combo->signal_changed().connect(sigc::mem_fun(*this,&VBrowser::on_sort_changed));
  Glib::ustring stock_icon = "view-sort-ascending";
  if (sort_desc) stock_icon="view-sort-descending";
  asc_button = new Gtk::Button();
  asc_button->set_image_from_icon_name(stock_icon,Gtk::ICON_SIZE_BUTTON);
  asc_button->signal_clicked().connect(sigc::mem_fun(*this,&VBrowser::asc_clicked));
  sort_opt->add(*sort_label);
  sort_opt->add(*sort_combo);
  sort_opt->add(*asc_button);
  sort_opt->pack_end(*fdupe_button,  false,  false,  0);
  sort_opt->pack_end(*browse_button,  false, false, 0 ); 
  box_outer->pack_start(*sort_opt, false, true, 0);
  fScrollWin = new Gtk::ScrolledWindow();
  box_outer->pack_start(*fScrollWin, true, true, 0);
  this->populate_icons();
  this->show_all();
}

VBrowser::~VBrowser() {
}

void VBrowser::populate_icons(bool clean) {
  if(clean) fScrollWin->remove();
  fFBox = new Gtk::FlowBox();
  fFBox->set_homogeneous(false);
  fFBox->set_sort_func(sigc::mem_fun(*this,&VBrowser::sort_videos));
  std::vector<bfs::directory_entry> video_list;
  std::vector<std::future<VideoIcon * > > icons;
  for (bfs::directory_entry & x : bfs::directory_iterator(path))
    {
      auto extension = x.path().extension().generic_string();
      std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
      if(extensions.count(extension)) {
	std::string pathName(x.path().native());
	//std::cout << "PATH: "<<pathName<<std::endl;
	icons.push_back(TPool->push([](std::string path,DbConnector * con) {return new VideoIcon(path,con);},pathName,dbCon));
      }
    }
  for(auto &a: icons) fFBox->add(*(a.get()));
  fFBox->invalidate_sort();
  fScrollWin->add(*fFBox);
}

std::string VBrowser::get_sort() {
  return sort_by;
}

void VBrowser::set_sort(std::string sort) {
  sort_by = sort;
}

int VBrowser::sort_videos(Gtk::FlowBoxChild *videoFile1, Gtk::FlowBoxChild *videoFile2) {
  int value=1;
  VidFile *v1=(reinterpret_cast<VideoIcon*>(videoFile1->get_child()))->get_vid_file();
  VidFile *v2=(reinterpret_cast<VideoIcon*>(videoFile2->get_child()))->get_vid_file();
  if(sort_desc) value *= -1;
  if(!std::strcmp(this->get_sort().c_str(),"size")) return value*(v1->size - v2->size);
  else if(!std::strcmp(this->get_sort().c_str(),"length")) return value*(v1->length - v2->length);
  else return value*boost::algorithm::to_lower_copy(v1->fileName).compare(boost::algorithm::to_lower_copy(v2->fileName));   
}

void VBrowser::browse_clicked() {
  Gtk::FileChooserDialog dialog("Please choose a folder", Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
  dialog.set_default_size(800, 400);
  dialog.set_transient_for(*this);
  dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dialog.add_button("Select", Gtk::RESPONSE_OK);
  auto response = dialog.run();
  if (response == Gtk::RESPONSE_OK){
    path  = dialog.get_filename()+"/";
    this->populate_icons(true);
  }
  this->show_all();
}

void VBrowser::on_delete() {
  dbCon->save_db_file();
}

void VBrowser::fdupe_clicked(){
  std::vector<VidFile *> videos;
  std::vector<int> vids;
  for (bfs::directory_entry & x : bfs::directory_iterator(path)) {
    auto extension = x.path().extension().generic_string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    if(extensions.count(extension)) {
      bfs::path full_path = bfs::absolute(x.path());
      VidFile * vid_obj = dbCon->fetch_video(full_path.c_str());
      int video_id = vid_obj->vid;
      vids.push_back(video_id);
      if(!dbCon->trace_exists(video_id)) videos.push_back(vid_obj);
    }
  }
  std::vector<std::future< bool > > jobs;
  for(auto & b: videos) {
    jobs.push_back(TPool->push([&](VidFile * b ){ return calculate_trace(b);},b));
  }
  cxxpool::wait(jobs.begin(),jobs.end());
  std::map<std::pair<int,int>,int> result_map;
  std::vector<unsigned short> vdat1, vdat2;
  bool match = false;
  int counter=0;
  dbCon->fetch_results(result_map);			
  //loop over files  TODO-make this parallel when we have larger sample
  for(int i = 0; i +1 < vids.size(); i++) {
    dbCon->fetch_trace(vids[i],vdat1);
    //loop over files after i
    for(int j = i+1; j < vids.size(); j++) {
      //std::cout << vids[i] << " "<<vids[j] << std::endl;
      if (result_map[std::make_pair(vids[i],vids[j])]) {
	std::cout <<"ALREADY Computed." <<std::endl;
	continue;
      }
      match=false;
      dbCon->fetch_trace(vids[j],vdat2);
      //std::cout <<"VDAT1 "<<vdat1.size()<<" "<< vdat1[0] <<" "<< vdat1[1] <<" "<< vdat1[2] <<" "<< vdat1[3] <<std::endl;
      //std::cout <<"VDAT2 "<<vdat2.size()<<" "<< vdat2[0] <<" "<< vdat2[1] <<" "<< vdat2[2] <<" "<< vdat2[3] <<std::endl;
      uint t_s,t_x, t_o, t_d;
      //loop over slices
      for(t_s =0; t_s < vdat1.size()-12*TRACE_FPS*COMP_TIME; t_s+= 12*TRACE_FPS*SLICE_SPACING){
	if(match) break;
	//starting offset for 2nd trace-this is the loop for the indiviual tests
	for(t_x=0; t_x < vdat2.size()-12*TRACE_FPS*COMP_TIME; t_x+=12){
	  if(match) break;
	  std::vector<int> accum(12);
	  //offset loop
	  for(t_o = 0; t_o < 12*COMP_TIME*TRACE_FPS; t_o+=12){
	    counter = 0;
	    for(auto & a : accum) if (a > THRESH) counter+=1;
	    if(counter != 0) break;
	    //pixel/color loop
	    for (t_d = 0; t_d < 12; t_d++) {
	      int value = pow((int)(vdat1[t_s+t_o+t_d])-(int)(vdat2[t_x+t_o+t_d]),2)-pow(FUDGE,2);
	      if(value < 0) value = 0;
	      accum[t_d]+=value;
	    }
	  }
	  counter = 0;
	  for(auto & a: accum)  if(a < THRESH) counter+=1;
	  if(counter == 12) match=true;
	  if(match) std::cout << "ACCUM " <<vids[i]<<" " <<vids[j] <<" " <<t_o <<" slice " <<t_s <<" 2nd offset " <<t_x <<" " <<accum[0]  <<std::endl;
	}
      }
      int result = 1;
      if (match) result = 2;
      dbCon->update_results(vids[i],vids[j],result);  
    }
  }
}

bool VBrowser::calculate_trace(VidFile * obj) {
  bfs::path path(obj->fileName);
  bfs::path imgp=HOME_PATH;
  imgp+=(boost::format("border%i.png") % obj->vid).str();
  std::cout <<obj->vid<<" "<< path <<" " << std::endl;
  double length = obj->length;
  double start_time = TRACE_TIME;
  if(length <= TRACE_TIME) start_time=0.0;
  double frame_time = start_time;
  double frame_spacing = (length-start_time)/BORDER_FRAMES;
  std::string cmdTmpl("ffmpeg -y -nostats -loglevel 0 -ss %.3f -i %s -frames:v 1 %s");
  std::string command((boost::format(cmdTmpl)% start_time % obj->fixed_filename() % imgp ).str());
  std::system(command.c_str());
  MagickWand *image_wand1=NewMagickWand();
  MagickWand *image_wand2=NewMagickWand();
  PixelIterator* iterator;
  PixelWand ** pixels;
  bool ok = MagickReadImage(image_wand2,imgp.string().c_str());
  ExceptionType severity;
  MagickPixelPacket pixel;
  unsigned long width,height;
  register long x;
  long y;
  height = MagickGetImageHeight(image_wand2);
  width = MagickGetImageWidth(image_wand2);
  std::vector<double> rowSums(height);
  std::vector<double> colSums(width);
  double corrFactorCol = 1.0/(double)(BORDER_FRAMES*height);
  double corrFactorRow = 1.0/(double)(BORDER_FRAMES*width);
  bool skipBorder = false;
  for(int i = 1; i < BORDER_FRAMES; i++) {
    image_wand1 = CloneMagickWand(image_wand2);
    frame_time+=frame_spacing;
    std::system((boost::format(cmdTmpl) % frame_time % obj->fixed_filename()  % imgp ).str().c_str());
    MagickReadImage(image_wand2,imgp.string().c_str());
    MagickCompositeImage(image_wand1,image_wand2,DifferenceCompositeOp, 0,0);
    iterator = NewPixelIterator(image_wand1);
    for (y=0; y < height; y++) {
      pixels=PixelGetNextIteratorRow(iterator,&width);
      if (pixels == (PixelWand **) NULL) break;
      for (x=0; x < (long) width; x++) {
	PixelGetMagickColor(pixels[x],&pixel);
	double value =sqrt(1.0/3.0*(pow(pixel.red,2.0)+pow(pixel.green,2.0)+pow(pixel.blue,2.0)));
	rowSums[y]+=corrFactorRow*value;
	colSums[x]+=corrFactorCol*value;
      }
    }
    if(rowSums[0] > CUT_THRESH &&
       rowSums[height-1] > CUT_THRESH &&
       colSums[0] > CUT_THRESH &&
       colSums[width-1] > CUT_THRESH) {
      skipBorder = true;
      break;
    }
  }
  bfs::remove(imgp);
  image_wand1=DestroyMagickWand(image_wand1);
  image_wand2=DestroyMagickWand(image_wand2);
  boost::process::ipstream is;
  if(skipBorder) {
    boost::process::system((boost::format("ffmpeg -y -nostats -loglevel 0 -ss %.3f -i \"%s\" -filter:v \"fps=%.3f,scale=2x2\" -pix_fmt rgb24 -f image2pipe -vcodec rawvideo - ") % start_time % obj->fixed_filename()  % TRACE_FPS).str(),boost::process::std_out > is);
  }
  else {  
    int x1(0), x2(width-1), y1(0), y2(height-1);
    while(colSums[x1] < CUT_THRESH) x1++;
    while(colSums[x2] < CUT_THRESH) x2--;
    while(rowSums[y1] < CUT_THRESH) y1++;
    while(rowSums[y2] < CUT_THRESH) y2--;
    x2-=x1-1;  
    y2-=y1-1;
    //std::cout << x2 << " "<<y2 <<" "<<x1 <<" "<<y1 << std::endl;
    boost::process::system((boost::format("ffmpeg -y -nostats -loglevel 0 -ss %.3f -i \"%s\" -filter:v \"fps=%.3f,crop=%i:%i:%i:%i,scale=2x2\" -pix_fmt rgb24 -f image2pipe -vcodec rawvideo - ") % start_time % obj->fixed_filename()  % TRACE_FPS % x2 % y2 % x1 % y1).str(),boost::process::std_out > is);
  }
  std::string outString;
  std::getline(is,outString);
  dbCon->save_trace(obj->vid, outString);
  return true; 
}

void VBrowser::on_sort_changed() {
  sort_by = sort_combo->get_active_text();
  fFBox->invalidate_sort();
}

void VBrowser::asc_clicked() {
  sort_desc=! sort_desc;
  Glib::ustring iname;
  if (sort_desc) iname = "view-sort-descending";
  else iname = "view-sort-ascending";
  asc_button->set_image_from_icon_name(iname,Gtk::ICON_SIZE_BUTTON);
  fFBox->invalidate_sort();
}

