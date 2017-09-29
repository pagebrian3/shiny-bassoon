#include "VBrowser.h"
#include <wand/MagickWand.h>
#include <boost/format.hpp>
#include <boost/process.hpp>
#include <set>

#define WIN_WIDTH 800
#define WIN_HEIGHT 600
#define FUDGE 8
#define TRACE_TIME 10.0
#define TRACE_FPS 25.0
#define BORDER_FRAMES 20.0
#define CUT_THRESH 1000.0
#define COMP_TIME 20
#define SLICE_SPACING 60
#define THRESH 1000.0*COMP_TIME*TRACE_FPS
#define EXTENSION ".png"
#define HOME_PATH "/home/ungermax/.video_proj/"
#define DEFAULT_PATH "/home/ungermax/mt_test/"
#define DEFAULT_SORT "size"
#define ICON_SIZE 6
#define DEFAULT_DESC true

std::set<std::string> extensions{".mp4",".wmv",".mov",".rm",".m4v",".flv",".avi",".qt",".mpg",".mpeg",".mpv",".3gp"};

VBrowser::VBrowser() {
  this->set_default_size(WIN_WIDTH, WIN_HEIGHT);
  dbCon = new DbConnector();
  Gtk::VBox * box_outer = new Gtk::VBox(false, 6);
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
  fFBox->set_sort_func(sigc::mem_fun(*this,&VBrowser::sort_videos));
  std::vector<bfs::directory_entry> video_list;
  for (bfs::directory_entry & x : bfs::recursive_directory_iterator(path))
    {
      auto extension = x.path().extension().generic_string();
      std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
      if(extensions.count(extension)) {
	std::string pathName(x.path().native());
	VideoIcon * icon = new VideoIcon(pathName.c_str(),dbCon);
	fFBox->add(*icon);
      }
    }
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
  if(!sort_desc) value *= -1;
  if(!std::strcmp(this->get_sort().c_str(),"size")) return value*(v1->size - v2->size);
  else if(!std::strcmp(this->get_sort().c_str(),"length")) return value*(v1->length - v2->length);
  else return value*v1->fileName.compare(v2->fileName);   
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
  for (bfs::directory_entry & x : bfs::recursive_directory_iterator(path))
    {
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
  for(auto & b: videos) calculate_trace(b);        
  std::map<std::pair<int,int>,int> result_map;
  std::vector<int> vdat1, vdat2;
  bool match = false;
  int counter=0;
  dbCon->fetch_results(result_map);			
  //loop over files  TODO-make this parallel when we have larger sample
  for(int i = 0; i +1 < vids.size(); i++) {
    dbCon->fetch_trace(vids[i],vdat1);
    //loop over files after i
    for(int j = i+1; j < vids.size(); j++) {
      if (result_map[std::make_pair(vids[i],vids[j])]) continue;
      match=false;
      dbCon->fetch_trace(vids[j],vdat2);
      uint t_s,t_x, t_o, t_d;
      //loop over slices
      for(t_s =0; t_s < vdat1.size()-12*TRACE_FPS*COMP_TIME; t_s+= 12*TRACE_FPS*SLICE_SPACING){
	if(match) break;
	//starting offset for 2nd trace -this is the loop for the indiviual tests
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
	      int value = pow(int(vdat1[t_s+t_o+t_d])-int(vdat2[t_x+t_o+t_d]),2)-pow(FUDGE,2);
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

void VBrowser::calculate_trace(VidFile * obj) {
  boost::filesystem::path path(obj->fileName);
  boost::filesystem::path temp_dir = boost::filesystem::unique_path();
  char home[100]=HOME_PATH;
  char img_path[100]="border.png";
  char imgp[100] =HOME_PATH;
  std::strcat(imgp,img_path);
  std::strcat(home,temp_dir.c_str());
  boost::filesystem::create_directory(home);
  boost::process::ipstream is;
  boost::process::system((boost::format("ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 %s" ) % path).str().c_str(),boost::process::std_out > is);
  std::string outString;
  std::getline(is,outString);
  double length = std::stod(outString);
  double start_time = TRACE_TIME;
  if(length <= TRACE_TIME) start_time=0.0; 
  double frame_spacing = (length-start_time)/BORDER_FRAMES;
  std::system((boost::format("ffmpeg -y -nostats -loglevel 0 -i \"%s\" -ss %.03d -vframes 1 %s") % path % start_time % imgp ).str().c_str());
  std::vector<boost::filesystem::directory_entry> bmpList;
  MagickWandGenesis();
  MagickWand *image_wand1=NewMagickWand();
  MagickWand *image_wand2=NewMagickWand();
  PixelIterator* iterator;
  PixelWand ** pixels;
  MagickReadImage(image_wand2,imgp);
  MagickPixelPacket pixel;
  std::vector<double> blah;
  unsigned long width,height;
  register long x;
  long y;
  height = MagickGetImageHeight(image_wand2);
  width = MagickGetImageWidth(image_wand2);
  //std::cout << "BLAH " << width <<" "<<height<<std::endl;
  std::vector<double> rowSums(height);
  std::vector<double> colSums(width);
  double corrFactorCol = 1.0/(double)(BORDER_FRAMES*height);
  double corrFactorRow = 1.0/(double)(BORDER_FRAMES*width);
  for(int i = 1; i < BORDER_FRAMES; i++) {
    image_wand1 = CloneMagickWand(image_wand2);
    start_time+=frame_spacing;
    std::system((boost::format("ffmpeg -y -nostats -loglevel 0 -i \"%s\" -ss %d -vframes 1 %s") % path % start_time % imgp ).str().c_str());
    MagickReadImage(image_wand2,imgp);
    MagickCompositeImage(image_wand1,image_wand2,DifferenceCompositeOp, 0,0);
    iterator = NewPixelIterator(image_wand1);
    for (y=0; y < height; y++)
      {
	//std::cout << x <<" "<<y << std::endl;
	pixels=PixelGetNextIteratorRow(iterator,&width);
	if (pixels == (PixelWand **) NULL) break;
	for (x=0; x < (long) width; x++)
	  {
	    PixelGetMagickColor(pixels[x],&pixel);
	    double value =sqrt(1.0/3.0*(pow(pixel.red,2.0)+pow(pixel.green,2.0)+pow(pixel.blue,2.0)));
	    rowSums[y]+=corrFactorRow*value;
	    colSums[x]+=corrFactorCol*value;
	  }
      }
    if(rowSums[0] > CUT_THRESH &&
       rowSums[height-1] > CUT_THRESH &&
       colSums[0] > CUT_THRESH &&
       colSums[width-1] > CUT_THRESH) break;
  }
  int x1(0), x2(width-1), y1(0), y2(height-1);
  while(colSums[x1] < CUT_THRESH) x1++;
  while(colSums[x2] < CUT_THRESH) x2--;
  while(rowSums[y1] < CUT_THRESH) y1++;
  while(rowSums[y2] < CUT_THRESH) y2--;
  x2-=x1-1;  //the -1 corrects for the fact that x2 and y2 are sizes, not positions
  y2-=y1-1;
  //std::cout << x2 << " "<<y2 <<" "<<x1 <<" "<<y1 << std::endl;
  boost::filesystem::remove(imgp);
  //std::cout << (boost::format("ffmpeg -y -nostats -loglevel 0 -i \"%s\" -ss %d -vf fps=%d -filter \"crop=%i:%i:%i:%i,scale=2x2\" %s/out%%05d%s") % path % TRACE_TIME % TRACE_FPS % x2 % y2 % x1 % y1 %home % EXTENSION).str().c_str();
  std::system((boost::format("ffmpeg -y -nostats -loglevel 0 -i \"%s\" -ss %d -vf fps=%d -filter \"crop=%i:%i:%i:%i,scale=2x2\" %s/out%%05d%s") % path % TRACE_TIME % TRACE_FPS % x2 % y2 % x1 % y1 %home % EXTENSION).str().c_str());
  bmpList.clear();
  boost::filesystem::path p(home);
  for (boost::filesystem::directory_entry & x : boost::filesystem::recursive_directory_iterator(p))  bmpList.push_back(x);
  std::sort(bmpList.begin(),bmpList.end());
  int listSize = bmpList.size();
  std::vector<int> data(12*listSize);
  int i = 0;
  double scale = 1.0/256.0;
  ClearMagickWand(image_wand1);
  for(auto tFile: bmpList) {
    auto status = MagickReadImage(image_wand1,tFile.path().c_str());
    iterator = NewPixelIterator(image_wand1);
    for (y=0; y < 2; y++)
      {
	pixels=PixelGetNextIteratorRow(iterator,&width);
	if (pixels == (PixelWand **) NULL) 
	  break;
	for (x=0; x < 2; x++)
	  {
	    PixelGetMagickColor(pixels[x],&pixel);    
	    data[12*i+6*y+3*x]=scale*pixel.red;
	    data[12*i+6*y+3*x+1]=scale*pixel.green;
	    data[12*i+6*y+3*x+2]=scale*pixel.blue;
	  }
      }
    i++;
  }
  dbCon->save_trace(obj->vid, data);  
  return;
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

