#include <DbConnector.h>
#include <VidFile.h>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <iostream>

DbConnector::DbConnector(bfs::path & appPath) {
  bfs::path db_path(appPath);
  icon_path = appPath;
  if(!bfs::exists(appPath)) bfs::create_directory(appPath);
  db_path+="vdb.db";
  bool newFile = true;
  if (bfs::exists(db_path)) newFile=false;
   sqlite3_open(db_path.c_str(),&db);
  if (newFile) {
    sqlite3_exec(db,"create table results(v1id integer, v2id integer, result integer)",NULL, NULL, NULL);
    sqlite3_exec(db,"create table icon_blobs(vid integer primary key, img_dat blob, FOREIGN KEY(vid) REFERENCES videos(vid))",NULL, NULL, NULL);
    sqlite3_exec(db,"create table videos(vid integer primary key not null, path text,crop text, length double, size integer, okflag integer, rotate integer, height integer, width integer)", NULL,NULL, NULL);
    sqlite3_exec(db,"create table config(cfg_label text primary key,cfg_type integer,cfg_int integer,cfg_float double,cfg_str text)",NULL,NULL,NULL);
    sqlite3_exec(db,"create table md_types(md_type_index integer primary key not null, md_type_label text unique)",NULL,NULL,NULL);
    sqlite3_exec(db,"create table md_index(md_idx integer primary key not null, md_type integer not null, md_label text unique)",NULL,NULL,NULL);
    sqlite3_exec(db,"create table file_mdx(vid integer primary key, tagids text,FOREIGN KEY(vid) REFERENCES videos(vid))",NULL,NULL,NULL);
  }
}

void DbConnector::save_db_file() {
  sqlite3_close(db);
}

VidFile * DbConnector::fetch_video(bfs::path & filename){
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "SELECT crop, length, size, okflag, rotate, vid, height, width FROM videos WHERE path = ? limit 1", -1, &stmt, NULL);
  if (rc != SQLITE_OK) throw std::string(sqlite3_errmsg(db));
  rc = sqlite3_bind_text(stmt, 1, filename.c_str(),-1, NULL);    
  if (rc != SQLITE_OK) {               
    std::string errmsg(sqlite3_errmsg(db)); 
    sqlite3_finalize(stmt);            
    throw errmsg;                      
  }
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
    std::string errmsg(sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    throw errmsg;
  }
  if (rc == SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw std::string("video not found");
  }
  std::string crop;
  try {
    crop.assign(reinterpret_cast<const char *>(sqlite3_column_text(stmt,0)));
  } catch(const std::exception& e) {
    std::cout << "Something happened for : " << filename.string() << std::endl;
  }
  float length = sqlite3_column_double(stmt,1);
  int size = sqlite3_column_int(stmt,2);
  int flag = sqlite3_column_int(stmt,3);
  int rotate = sqlite3_column_int(stmt,4);
  int vid = sqlite3_column_int(stmt,5);
  int height = sqlite3_column_int(stmt,6);
  int width = sqlite3_column_int(stmt,7);
  sqlite3_finalize(stmt); 
  VidFile * result = new VidFile(filename, length, size, flag, vid, crop, rotate, height, width);
  return result;
}

bfs::path DbConnector::fetch_icon(int vid){
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "SELECT img_dat FROM icon_blobs WHERE vid = ? limit 1", -1, &stmt, NULL);
  if (rc != SQLITE_OK) throw std::string(sqlite3_errmsg(db));
  rc = sqlite3_bind_int(stmt, 1, vid);    
  if (rc != SQLITE_OK) {               
    std::string errmsg(sqlite3_errmsg(db)); 
    sqlite3_finalize(stmt);            
    throw errmsg;                      
  }
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
    std::string errmsg(sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    throw errmsg;
  }
  bfs::path out_path = createPath(icon_path,vid,".jpg");
  std::ofstream output(out_path.string(), std::ios::out|std::ios::binary|std::ios::ate);
  int size = sqlite3_column_bytes(stmt,0);
  output.write((char*)sqlite3_column_blob(stmt,0),size);
  sqlite3_finalize(stmt);
  output.close();
  return bfs::path(out_path);
}

void DbConnector::save_video(VidFile* a) {
  a->okflag=1;
  std::string crop_holder("");
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "INSERT INTO videos(path,crop,length,size,okflag,rotate,width,height) VALUES (?,?,?,?,?,?,?,?) ", -1, &stmt, NULL);
  if (rc != SQLITE_OK) throw std::string(sqlite3_errmsg(db));
  sqlite3_bind_text(stmt, 1, a->fileName.c_str(), -1, NULL);
  sqlite3_bind_text(stmt, 2, crop_holder.c_str(), -1, NULL);
  sqlite3_bind_double(stmt, 3, a->length);
  sqlite3_bind_int(stmt, 4, a->size);
  sqlite3_bind_int(stmt, 5, a->okflag);
  sqlite3_bind_int(stmt, 6, a->rotate);
  sqlite3_bind_int(stmt, 7, a->width);
  sqlite3_bind_int(stmt, 8, a->height);
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
    std::string errmsg(sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    throw errmsg;
  } 
  sqlite3_finalize(stmt);
  a->vid=sqlite3_last_insert_rowid(db);
  return;
}

void DbConnector::save_crop(VidFile* a) {
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "UPDATE videos SET crop=? WHERE vid=?", -1, &stmt, NULL);
  if (rc != SQLITE_OK) throw std::string(sqlite3_errmsg(db));
  sqlite3_bind_text(stmt, 1, a->crop.c_str(), -1, NULL);
  sqlite3_bind_int(stmt, 2, a->vid);
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
    std::string errmsg(sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    throw errmsg;
  } 
  sqlite3_finalize(stmt);
  return;
}

bfs::path DbConnector::save_icon(int vid) {  
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "INSERT INTO icon_blobs (vid,img_dat) VALUES (? , ?) ", -1, &stmt, NULL);
  if (rc != SQLITE_OK) throw std::string(sqlite3_errmsg(db));
  rc = sqlite3_bind_int(stmt, 1, vid);
  bfs::path in_path = createPath(icon_path,vid,".jpg");
  std::ifstream input(in_path.string(), std::ios::in|std::ios::binary|std::ios::ate);
  unsigned char * memblock;
  int length;
  if (input.is_open()) {
    std::streampos size = input.tellg();
    memblock = (unsigned char *)malloc(size);
    input.seekg (0, std::ios::beg);
    input.read (reinterpret_cast<char *>(memblock), size);
    input.close();
    length = sizeof(unsigned char)*size;
  }
  else {
    std::cout << "File empty/not there. vid: " << vid << " path: " << in_path << " error: " << strerror(errno) << std::endl;
    exit(EXIT_FAILURE);
  }
  rc = sqlite3_bind_blob(stmt, 2, memblock,length, SQLITE_STATIC);
  if (rc != SQLITE_OK) {               
    std::string errmsg(sqlite3_errmsg(db)); 
    sqlite3_finalize(stmt);            
    throw errmsg;                      
  }
  rc = sqlite3_step(stmt); 
  sqlite3_finalize(stmt);
  delete[] memblock;
  return in_path;
}

bool DbConnector::video_exists(bfs::path & filename){
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "SELECT EXISTS(SELECT 1 FROM videos WHERE path = ? limit 1)", -1, &stmt, NULL);
  if (rc != SQLITE_OK) throw std::string(sqlite3_errmsg(db));
  rc = sqlite3_bind_text(stmt, 1, filename.c_str(),-1, NULL);    
  if (rc != SQLITE_OK) {               
    std::string errmsg(sqlite3_errmsg(db)); 
    sqlite3_finalize(stmt);            
    throw errmsg;                      
  }
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
    std::string errmsg(sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    throw errmsg;
  }
  if (rc == SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw std::string("video not found");
  }
  bool result = sqlite3_column_int(stmt, 0);
  sqlite3_finalize(stmt);
  return result;
}

bool DbConnector::icon_exists(int vid){
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "SELECT EXISTS(SELECT 1 FROM icon_blobs WHERE vid = ? limit 1)", -1, &stmt, NULL);
  if (rc != SQLITE_OK) throw std::string(sqlite3_errmsg(db));
  rc = sqlite3_bind_int(stmt, 1, vid);    
  if (rc != SQLITE_OK) {               
    std::string errmsg(sqlite3_errmsg(db)); 
    sqlite3_finalize(stmt);            
    throw errmsg;                      
  }
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
    std::string errmsg(sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    throw errmsg;
  }
  if (rc == SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw std::string("video not found");
  }
  bool result = sqlite3_column_int(stmt, 0);
  sqlite3_finalize(stmt);
  return result;
}
  
void DbConnector::fetch_results(std::map<std::pair<int,int>, int> & map) {
  sqlite3_stmt *cstmt;
  int rc = sqlite3_prepare_v2(db,"SELECT Count(*) FROM results", -1, &cstmt, NULL);
  sqlite3_step(cstmt);
  if (sqlite3_column_int(cstmt, 0) == 0) {
    sqlite3_finalize(cstmt);
    return;
  }
  sqlite3_finalize(cstmt);
  sqlite3_stmt *stmt;
  rc = sqlite3_prepare_v2(db, "SELECT * FROM results", -1, &stmt, NULL);
  if (rc != SQLITE_OK) throw std::string(sqlite3_errmsg(db));
  rc = 0; 
  int a,b,c;
  while((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    a = sqlite3_column_int(stmt, 0);
    b = sqlite3_column_int(stmt, 1);
    c = sqlite3_column_int(stmt, 2);
    map[std::make_pair(a,b)]=c;
  }
  if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
    std::string errmsg(sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    throw errmsg;
  }
  sqlite3_finalize(stmt);
  return;
}

void DbConnector::update_results(int  i, int  j, int  k) {
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "INSERT or REPLACE INTO results (v1id, v2id, result) VALUES (?,?,?) ", -1, &stmt, NULL);
  if (rc != SQLITE_OK) throw std::string(sqlite3_errmsg(db));
  rc = sqlite3_bind_int(stmt, 1, i);
  rc = sqlite3_bind_int(stmt, 2, j);
  rc = sqlite3_bind_int(stmt, 3, k);
  if (rc != SQLITE_OK) {               
    std::string errmsg(sqlite3_errmsg(db)); 
    sqlite3_finalize(stmt);            
    throw errmsg;                      
  }
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
    std::string errmsg(sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    throw errmsg;
  } 
  sqlite3_finalize(stmt);
  return;
}

bfs::path DbConnector::createPath(bfs::path & path, int vid, std::string extension) {
  return bfs::path((boost::format("%s%i%s") % path.string() % vid % extension).str());
}

void DbConnector::cleanup(bfs::path & dir, std::vector<bfs::path> & files){
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db,(boost::format("SELECT path,vid FROM videos WHERE path LIKE '%s%%'") % dir.string()).str().c_str(), -1, &stmt, NULL);
  if (rc != SQLITE_OK) throw std::string(sqlite3_errmsg(db));
  if (rc != SQLITE_OK) {               
    std::string errmsg(sqlite3_errmsg(db)); 
    sqlite3_finalize(stmt);            
    throw errmsg;                      
  }
  std::map<int,bfs::path> videos;
  while((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    bfs::path path1(reinterpret_cast<const char *>(sqlite3_column_text(stmt,0)));
    int vid = sqlite3_column_int(stmt,1);
    videos[vid]=path1;
  }
  sqlite3_finalize(stmt);
  std::vector<int> orphans;
  for(auto & a: videos) {
    bool match = false;
    for(auto & b: files)   
      if(a.second==b) match=true;
    if(!match) orphans.push_back(a.first);
  }
  for(auto & a: orphans) {
    rc = sqlite3_prepare_v2(db, "DELETE FROM icon_blobs WHERE vid = ?", -1, &stmt, NULL);
    rc = sqlite3_bind_int(stmt, 1, a); 
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    rc = sqlite3_prepare_v2(db, "DELETE FROM videos WHERE vid = ?", -1, &stmt, NULL);
    rc = sqlite3_bind_int(stmt, 1, a); 
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
     rc = sqlite3_prepare_v2(db, "DELETE FROM results WHERE v1id = ? OR v2id = ?", -1, &stmt, NULL);
    rc = sqlite3_bind_int(stmt, 1, a); 
    rc = sqlite3_bind_int(stmt, 2, a); 
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
  }  
  return;
}

std::vector<std::pair<std::string,boost::variant<int,float,std::string>> >DbConnector::fetch_config() {
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db,"SELECT cfg_label, cfg_type, cfg_int, cfg_float, cfg_str FROM config" , -1, &stmt, NULL);
  if (rc != SQLITE_OK) {               
    std::string errmsg(sqlite3_errmsg(db)); 
    sqlite3_finalize(stmt);            
    throw errmsg;                      
  }
  std::vector<std::pair<std::string,boost::variant<int,float,std::string> > > configs;
  while((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    std::string label(reinterpret_cast<const char *>(sqlite3_column_text(stmt,0)));
    int var_type = sqlite3_column_int(stmt,1);
    int var_i = sqlite3_column_int(stmt,2);
    float var_f = sqlite3_column_double(stmt,3);
    std::string var_s(reinterpret_cast<const char *>(sqlite3_column_text(stmt,4)));
    boost::variant<int,float,std::string> val;
    if(var_type == 0)  val = var_i;
    else if(var_type ==  1) val = var_f;
    else if(var_type == 2) val = var_s;
    configs.push_back(std::make_pair(label,val));
  }
  sqlite3_finalize(stmt);
  return configs;
}

void DbConnector::save_config(std::map<std::string, boost::variant<int,float,std::string> > config) {
  for( auto &a: config) {
    int var_type = a.second.which();
    int val_i=0;
    float val_f = 0.0;
    std::string val_s = "";
    if(var_type == 0)  val_i= boost::get<int>(a.second);
    else if(var_type ==  1) val_f= boost::get<float>(a.second);
    else if(var_type == 2) val_s= boost::get<std::string>(a.second);
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, "INSERT or REPLACE INTO config (cfg_label, cfg_type,cfg_int, cfg_float, cfg_str) VALUES (?,?,?,?,?)", -1, &stmt, NULL);
    if (rc != SQLITE_OK) throw std::string(sqlite3_errmsg(db));
     sqlite3_bind_text(stmt, 1, a.first.c_str(),-1, NULL);
     sqlite3_bind_int(stmt, 2, var_type);
     sqlite3_bind_int(stmt, 3, val_i);
     sqlite3_bind_double(stmt, 4,val_f) ;
     sqlite3_bind_text(stmt, 5, val_s.c_str(),-1, NULL);
    if (rc != SQLITE_OK) {               
      std::string errmsg(sqlite3_errmsg(db)); 
      sqlite3_finalize(stmt);            
      throw errmsg;                      
    }
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
      std::string errmsg(sqlite3_errmsg(db));
      sqlite3_finalize(stmt);
      throw errmsg;
    }
    sqlite3_finalize(stmt);
  }
}

void DbConnector::load_metadata_labels(std::map<int,std::pair<int,std::string> > & labelLookup, boost::bimap<int, std::string> & typeLookup) {
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db,"SELECT md_idx,md_type,md_label from md_index" , -1, &stmt, NULL);
    if (rc != SQLITE_OK) {               
    std::string errmsg(sqlite3_errmsg(db)); 
    sqlite3_finalize(stmt);            
    throw errmsg;                      
  }
  while((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    int index = sqlite3_column_int(stmt,0);
    int type = sqlite3_column_int(stmt,1);
    std::string label(reinterpret_cast<const char *>(sqlite3_column_text(stmt,2)));
    labelLookup[index]=std::make_pair(type,label);
  }
  sqlite3_finalize(stmt);
   sqlite3_stmt *stmt1;
  rc = sqlite3_prepare_v2(db,"SELECT md_type_index ,md_type_label from md_types" , -1, &stmt1, NULL);
    if (rc != SQLITE_OK) {          
    std::string errmsg(sqlite3_errmsg(db)); 
    sqlite3_finalize(stmt1);            
    throw errmsg;                      
  }
  while((rc = sqlite3_step(stmt1)) == SQLITE_ROW) {
    int index = sqlite3_column_int(stmt1,0);
    std::string label(reinterpret_cast<const char *>(sqlite3_column_text(stmt1,1)));
    typeLookup.insert({index,label});
  }
  sqlite3_finalize(stmt1);
  return;
}

void DbConnector::load_metadata_for_files(std::vector<int> & vids,std::map<int,std::vector<int > > & fileMetadata) {
  for(auto & vid: vids) {
    if(!video_has_md(vid)) continue;
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, "SELECT tagids FROM file_mdx WHERE vid=? limit 1", -1, &stmt, NULL);
    if (rc != SQLITE_OK) throw std::string(sqlite3_errmsg(db));
    rc = sqlite3_bind_int(stmt, 1, vid);    
    if (rc != SQLITE_OK) {               
      std::string errmsg(sqlite3_errmsg(db)); 
      sqlite3_finalize(stmt);            
      throw errmsg;                      
    }
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
      std::string errmsg(sqlite3_errmsg(db));
      sqlite3_finalize(stmt);
      throw errmsg;
    }
    std::string tag_str(reinterpret_cast<const char *>(sqlite3_column_text(stmt,0)));
    std::vector<std::string> split_vec;
    boost::algorithm::split(split_vec, tag_str, boost::is_any_of(","));
    std::vector<int> tag_ids;
    for(auto & s: split_vec) tag_ids.push_back(std::atoi(s.c_str()));
    fileMetadata[vid]=tag_ids;
    sqlite3_finalize(stmt); 
  }
  return;
}

void DbConnector::save_metadata(std::map<int,std::vector<int > > & file_metadata,std::map<int,std::pair<int,std::string> > & labelLookup, boost::bimap<int, std::string> & typeLookup) {
  for(auto & a: typeLookup.left) {
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, "INSERT or REPLACE INTO  md_types(md_type_index, md_type_label) VALUES (?,?) ", -1, &stmt, NULL);
    if (rc != SQLITE_OK) throw std::string(sqlite3_errmsg(db));
    rc = sqlite3_bind_int(stmt, 1, a.first);
    rc = sqlite3_bind_text(stmt, 2, a.second.c_str(),-1, NULL);
    if (rc != SQLITE_OK) {               
      std::string errmsg(sqlite3_errmsg(db)); 
      sqlite3_finalize(stmt);            
      throw errmsg;                      
    }
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
      std::string errmsg(sqlite3_errmsg(db));
      sqlite3_finalize(stmt);
      throw errmsg;
    } 
    sqlite3_finalize(stmt);
  }
  for(auto & a: labelLookup) {
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, "INSERT or REPLACE INTO  md_index(md_idx, md_type,md_label) VALUES (?,?,?) ", -1, &stmt, NULL);
    if (rc != SQLITE_OK) throw std::string(sqlite3_errmsg(db));
    rc = sqlite3_bind_int(stmt, 1, a.first);
    rc = sqlite3_bind_int(stmt, 2, a.second.first);
    rc = sqlite3_bind_text(stmt, 3, a.second.second.c_str(),-1, NULL);
    if (rc != SQLITE_OK) {               
      std::string errmsg(sqlite3_errmsg(db)); 
      sqlite3_finalize(stmt);            
      throw errmsg;                      
    }
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
      std::string errmsg(sqlite3_errmsg(db));
      sqlite3_finalize(stmt);
      throw errmsg;
    } 
    sqlite3_finalize(stmt); 
  }
  for(auto & a: file_metadata) {
    if(a.second.size() == 0)  continue;
    sqlite3_stmt *stmt;
    std::stringstream ss;
    int counter=0;
    while(counter < a.second.size()-1) {
      ss << a.second[counter] << ",";
      counter++;
    }
    ss << a.second[counter]<<std::endl;   
    int rc = sqlite3_prepare_v2(db, "INSERT or REPLACE INTO  file_mdx(vid, tagids) VALUES (?,?) ", -1, &stmt, NULL);
    if (rc != SQLITE_OK) throw std::string(sqlite3_errmsg(db));
    rc = sqlite3_bind_int(stmt, 1, a.first);
    rc = sqlite3_bind_text(stmt, 2, ss.str().c_str(),-1, NULL);
    if (rc != SQLITE_OK) {               
      std::string errmsg(sqlite3_errmsg(db)); 
      sqlite3_finalize(stmt);            
      throw errmsg;                      
    }
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
      std::string errmsg(sqlite3_errmsg(db));
      sqlite3_finalize(stmt);
      throw errmsg;
    } 
    sqlite3_finalize(stmt); 
  } 
  return;
}

bool DbConnector::video_has_md(int vid){
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "SELECT EXISTS(SELECT 1 FROM file_mdx WHERE vid=? limit 1)", -1, &stmt, NULL);
  if (rc != SQLITE_OK) throw std::string(sqlite3_errmsg(db));
  rc = sqlite3_bind_int(stmt, 1, vid);    
  if (rc != SQLITE_OK) {               
    std::string errmsg(sqlite3_errmsg(db)); 
    sqlite3_finalize(stmt);            
    throw errmsg;                      
  }
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
    std::string errmsg(sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    throw errmsg;
  }
  if (rc == SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw std::string("video not found");
  }
  bool result = sqlite3_column_int(stmt, 0);
  sqlite3_finalize(stmt);
  return result;
}

