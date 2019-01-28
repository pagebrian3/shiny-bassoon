#include <DbConnector.h>
#include <boost/format.hpp>

DbConnector::DbConnector(bfs::path & appPath) {
  std::string db_path(appPath.c_str());
  icon_path = appPath;
  if(!bfs::exists(appPath)) bfs::create_directory(appPath);
  db_path.append("vdb.db");
  bool newFile = true;
  if (bfs::exists(db_path)) newFile=false;
   sqlite3_open(db_path.c_str(),&db);
  if (newFile) {
    sqlite3_exec(db,"create table results(v1id integer, v2id integer, result integer)",NULL, NULL, NULL);
    sqlite3_exec(db,"create table icon_blobs(vid integer primary key, img_dat blob)",NULL, NULL, NULL);
    sqlite3_exec(db,"create table trace_blobs(vid integer primary key,  uncomp_size integer, trace_dat text)", NULL,NULL, NULL);
    sqlite3_exec(db,"create table videos(vid integer primary key not null, path text,crop text, length double, size integer, okflag integer, rotate integer)", NULL,NULL, NULL);
    sqlite3_exec(db,"create table config(cfg_label text primary key, cfg_type integer, cfg_int integer, cfg_float double, cfg_str text)",NULL,NULL,NULL);
  }
}

void DbConnector::save_db_file() {
  sqlite3_close(db);
}

VidFile * DbConnector::fetch_video(bfs::path & filename){
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "SELECT crop, length, size, okflag, rotate, vid FROM videos WHERE path = ? limit 1", -1, &stmt, NULL);
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
  std::string crop(reinterpret_cast<const char *>(sqlite3_column_text(stmt,0)));
  float length = sqlite3_column_double(stmt,1);
  int size = sqlite3_column_int(stmt,2);
  int flag = sqlite3_column_int(stmt,3);
  int rotate = sqlite3_column_int(stmt,4);
  int vid = sqlite3_column_int(stmt,5);
  sqlite3_finalize(stmt); 
  VidFile * result = new VidFile(filename, length, size, flag, vid, crop, rotate);
  return result;
}

std::string DbConnector::fetch_icon(int vid){
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
  std::string out_path = create_icon_path(vid);
  std::ofstream output(out_path, std::ios::out|std::ios::binary|std::ios::ate);
  int size = sqlite3_column_bytes(stmt,0);
  output.write((char*)sqlite3_column_blob(stmt,0),size);
  sqlite3_finalize(stmt);
  output.close();
  return out_path;
}

void DbConnector::save_video(VidFile* a) {
  a->okflag=1;
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "INSERT INTO videos(path,length,size,okflag,rotate) VALUES (?,?,?,?,?) ", -1, &stmt, NULL);
  if (rc != SQLITE_OK) throw std::string(sqlite3_errmsg(db));
  sqlite3_bind_text(stmt, 1, a->fileName.c_str(), -1, NULL);
  sqlite3_bind_double(stmt, 2, a->length);
  sqlite3_bind_int(stmt, 3, a->size);
  sqlite3_bind_int(stmt, 4, a->okflag);
  sqlite3_bind_int(stmt, 5, a->rotate);
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

void DbConnector::save_icon(int vid) {  
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "INSERT INTO icon_blobs (vid,img_dat) VALUES (? , ?) ", -1, &stmt, NULL);
  if (rc != SQLITE_OK) throw std::string(sqlite3_errmsg(db));
  rc = sqlite3_bind_int(stmt, 1, vid);
  std::string in_path = create_icon_path(vid);
  std::ifstream input (in_path, std::ios::in|std::ios::binary|std::ios::ate);
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
    std::cout << "File empty/not there. vid: " <<vid<< std::endl;
    return;
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
  return;
}

bool DbConnector::trace_exists(int vid){
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "SELECT EXISTS(SELECT 1 FROM trace_blobs WHERE vid = ? limit 1)", -1, &stmt, NULL);
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
    throw std::string("trace not found");
  }
  bool result = sqlite3_column_int(stmt, 0);
  sqlite3_finalize(stmt);
  return result;
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
  
void DbConnector::fetch_trace(int vid, std::vector<uint8_t> & trace) {
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "SELECT uncomp_size,trace_dat FROM trace_blobs WHERE vid = ? limit 1", -1, &stmt, NULL);
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
  unsigned long int uncomp_size = sqlite3_column_int(stmt,0);
  std::string result;
  result.assign(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)));
  trace.resize(uncomp_size);
  for(int i = 0; i < uncomp_size; i++) trace[i] = result[i];
  sqlite3_finalize(stmt);
  return;
}

void DbConnector::save_trace(int  vid, std::string & trace) {
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "INSERT INTO trace_blobs (vid,uncomp_size,trace_dat) VALUES (?,?,?)", -1, &stmt, NULL);
  if (rc != SQLITE_OK) throw std::string(sqlite3_errmsg(db));
  rc = sqlite3_bind_int(stmt, 1, vid);
  unsigned long size = trace.length();
  sqlite3_bind_int(stmt, 2, size);
  sqlite3_bind_blob(stmt,3, trace.c_str(),size, NULL); 
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

std::string DbConnector::create_icon_path(int vid) {
  return (boost::format("%s%i.jpg") % icon_path.string() % vid).str();
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
    rc = sqlite3_prepare_v2(db, "DELETE FROM trace_blobs WHERE vid = ?", -1, &stmt, NULL);
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

