#include <DbConnector.h>
#include <boost/format.hpp>

DbConnector::DbConnector(bfs::path appPath) {
  bool newFile = true;
  bfs::path db_path = appPath;
  bfs::path icon_path = appPath;
  if(!bfs::exists(appPath)) bfs::create_directory(appPath);
  db_path+="vdb.sql";
  temp_icon = new char[icon_path.size()];
  strcpy(temp_icon, icon_path.c_str());
  if (bfs::exists(db_path)) newFile=false;
  sqlite3_open(db_path.c_str(),&db);
  if (newFile) {
    int rc = sqlite3_exec(db,"create table results(v1id integer, v2id integer, result integer)",NULL, NULL, NULL);
    sqlite3_exec(db,"create table icon_blobs(vid integer primary key, img_dat blob)",NULL, NULL, NULL);
    sqlite3_exec(db,"create table trace_blobs(vid integer primary key,  uncomp_size integer, trace_dat text)", NULL,NULL, NULL);
    sqlite3_exec(db,"create table videos(path text,crop text, length double, size integer, okflag integer, rotate integer, vdatid integer)", NULL,NULL, NULL);
  }
}

void DbConnector::save_db_file() {
  sqlite3_close(db);
}

VidFile * DbConnector::fetch_video(std::string  filename){
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "SELECT crop, length, size, okflag, rotate, vdatid FROM videos WHERE path = ? limit 1", -1, &stmt, NULL);
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
    throw std::string("something not found");
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

void DbConnector::fetch_icon(int vid){
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
  return;
}

void DbConnector::save_video(VidFile* a) {
  a->okflag=1;
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "INSERT INTO videos(path,length,size,okflag,rotate, vdatid) VALUES (?,?,?,?,?,?) ", -1, &stmt, NULL);
  if (rc != SQLITE_OK) throw std::string(sqlite3_errmsg(db));
  sqlite3_bind_text(stmt, 1, a->fileName.c_str(), -1, NULL);
  sqlite3_bind_double(stmt, 2, a->length);
  sqlite3_bind_int(stmt, 3, a->size);
  sqlite3_bind_int(stmt, 4, a->okflag);
  sqlite3_bind_int(stmt, 5, a->rotate);
  sqlite3_bind_int(stmt, 6, a->vid);
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
    std::string errmsg(sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    throw errmsg;
  } 
  sqlite3_finalize(stmt);
  return;
}

void DbConnector::save_crop(VidFile* a) {
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "UPDATE videos SET crop=? WHERE vdatid=?", -1, &stmt, NULL);
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

bool DbConnector::video_exists(std::string filename){
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

int DbConnector::get_last_vid(){
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "SELECT MAX(vid) from icon_blobs", -1, &stmt, NULL);
  if (rc != SQLITE_OK) throw std::string(sqlite3_errmsg(db));
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
    std::string errmsg(sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    throw errmsg;
  }
  if (rc == SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw std::string("entry not found");
  }

  int result = sqlite3_column_int(stmt, 0);

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
  int rc = sqlite3_prepare_v2(db, "INSERT INTO results (v1id, v2id, result) VALUES (?,?,?) ", -1, &stmt, NULL);
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
  
void DbConnector::fetch_trace(int vid, std::vector<unsigned short> & trace) {
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
  for(int i = 0; i < uncomp_size; i++)  {
    unsigned short value = result[i];
    if(value > 255) value = 256-(65536-value);
    trace[i] = value;
  }
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
  return (boost::format("%s%i.jpg") % temp_icon % vid).str();
}

