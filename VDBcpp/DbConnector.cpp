#include <DbConnector.h>
#include <sstream>
#include <boost/tokenizer.hpp>
#include <iostream>

DbConnector::DbConnector() {
  bool newFile = true;
  bfs::path path = "/home/ungermax/.video_proj/";
  bfs::path db_path = path;
  bfs::path icon_path = path;
  db_path+="vdb.sql";
  icon_path+="temp.png";
  temp_icon = new char[icon_path.size()];
  strcpy(temp_icon, icon_path.c_str());
  if (bfs::exists(db_path)) newFile=false;
  sqlite3_open(db_path.c_str(),&db);
  if (newFile) {
    min_vid=1;
    int rc = sqlite3_exec(db,"create table results(v1id integer, v2id integer, result integer)",NULL, NULL, NULL);
    sqlite3_exec(db,"create table icon_blobs(vid integer primary key, img_dat blob)",NULL, NULL, NULL);
    sqlite3_exec(db,"create table trace_blobs(vid integer primary key, trace_dat blob)", NULL,NULL, NULL);
    sqlite3_exec(db,"create table videos(path text,length integer, size integer, okflag integer, vdatid integer)", NULL,NULL, NULL);
  }
  else min_vid=this->get_last_vid();
}

void DbConnector::save_db_file() {
  sqlite3_close(db);
}

VidFile * DbConnector::fetch_video(std::string filename){
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "SELECT length, size, okflag, vdatid FROM videos WHERE path = ? limit 1", -1, &stmt, NULL);
  if (rc != SQLITE_OK)
    throw std::string(sqlite3_errmsg(db));
  rc = sqlite3_bind_text(stmt, 1, filename.c_str(),-1, NULL);    // Using parameters ("?") is not
  if (rc != SQLITE_OK) {                 // really necessary, but recommended
    std::string errmsg(sqlite3_errmsg(db)); // (especially for strings) to avoid
    sqlite3_finalize(stmt);            // formatting problems and SQL
    throw errmsg;                      // injection attacks.
  }
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
    std::string errmsg(sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    throw errmsg;
  }
  if (rc == SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw std::string("customer not found");
  }
   

  int length = sqlite3_column_int(stmt,0);
  int size = sqlite3_column_int(stmt,1);
  int flag = sqlite3_column_int(stmt,2);
  int vid = sqlite3_column_int(stmt,3);
  sqlite3_finalize(stmt);
  std::cout << "VID "<<vid<<std::endl;
  
  VidFile * result = new VidFile(filename, length, size, flag, vid);
  return result;
}

void DbConnector::fetch_icon(int vid){
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "SELECT img_dat FROM icon_blobs WHERE vid = ? limit 1", -1, &stmt, NULL);
  if (rc != SQLITE_OK)
    throw std::string(sqlite3_errmsg(db));
  rc = sqlite3_bind_int(stmt, 1, vid);    // Using parameters ("?") is not
  if (rc != SQLITE_OK) {                 // really necessary, but recommended
    std::string errmsg(sqlite3_errmsg(db)); // (especially for strings) to avoid
    sqlite3_finalize(stmt);            // formatting problems and SQL
    throw errmsg;                      // injection attacks.
  }
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
    std::string errmsg(sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    throw errmsg;
  }   
  std::ofstream output(temp_icon, std::ios::out|std::ios::binary|std::ios::ate);
  int size = sqlite3_column_bytes(stmt,0);
  std::cout << "Size RC vid" << size <<" " <<rc<<" "<<vid<< std::endl;
  output.write((char*)sqlite3_column_blob(stmt,0),size);
  sqlite3_finalize(stmt);
  output.close();
  return;
}

void DbConnector::save_video(VidFile* a) {
  a->vid=min_vid;
  min_vid++;
  a->okflag=1;
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "INSERT INTO videos(path,length,size,okflag,vdatid) VALUES (?,?,?,?,?) ", -1, &stmt, NULL);
  if (rc != SQLITE_OK) throw std::string(sqlite3_errmsg(db));
  sqlite3_bind_text(stmt, 1, a->fileName.c_str(), -1, NULL);
  sqlite3_bind_int(stmt, 2, a->length);
  sqlite3_bind_int(stmt, 3, a->size);
  sqlite3_bind_int(stmt, 4, a->okflag);
  sqlite3_bind_int(stmt, 5, a->vid);
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
  if (rc != SQLITE_OK)
    throw std::string(sqlite3_errmsg(db));
  rc = sqlite3_bind_int(stmt, 1, vid);
  std::ifstream input (temp_icon, std::ios::in|std::ios::binary|std::ios::ate);
  unsigned char * memblock;
  int length;
  if (input.is_open())
    {
      std::streampos size = input.tellg();
      memblock = (unsigned char *)malloc(size);
      input.seekg (0, std::ios::beg);
      input.read (reinterpret_cast<char *>(memblock), size);
      input.close();
      length = sizeof(unsigned char)*size;
      std::cout << "size , length: " << size <<" , "<<length << std::endl;
    }  
  rc = sqlite3_bind_blob(stmt, 2, memblock,length, SQLITE_STATIC);// Using parameters ("?") is not
  if (rc != SQLITE_OK) {                 // really necessary, but recommended
    std::string errmsg(sqlite3_errmsg(db)); // (especially for std::strings) to avoid
    sqlite3_finalize(stmt);            // formatting problems and SQL
    throw errmsg;                      // injection attacks.
  }
  rc = sqlite3_step(stmt); 
  sqlite3_finalize(stmt);
  if (sizeof(*memblock) > 4) delete[] memblock;
  return;
}

bool DbConnector::trace_exists(int vid){
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "SELECT EXISTS(SELECT 1 FROM trace_blobs WHERE vid = ? limit 1)", -1, &stmt, NULL);
  if (rc != SQLITE_OK)
    throw std::string(sqlite3_errmsg(db));
  rc = sqlite3_bind_int(stmt, 1, vid);    // Using parameters ("?") is not
  if (rc != SQLITE_OK) {                 // really necessary, but recommended
    std::string errmsg(sqlite3_errmsg(db)); // (especially for std::strings) to avoid
    sqlite3_finalize(stmt);            // formatting problems and SQL
    throw errmsg;                      // injection attacks.
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
  bool result = sqlite3_column_text(stmt, 0);
  sqlite3_finalize(stmt);
  return result;
}

bool DbConnector::video_exists(std::string filename){
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "SELECT EXISTS(SELECT 1 FROM videos WHERE path = ? limit 1)", -1, &stmt, NULL);
  if (rc != SQLITE_OK) throw std::string(sqlite3_errmsg(db));
  rc = sqlite3_bind_text(stmt, 1, filename.c_str(),-1, NULL);    // Using parameters ("?") is not
  if (rc != SQLITE_OK) {                 // really necessary, but recommended
    std::string errmsg(sqlite3_errmsg(db)); // (especially for std::strings) to avoid
    sqlite3_finalize(stmt);            // formatting problems and SQL
    throw errmsg;                      // injection attacks.
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
  if (rc != SQLITE_OK)
    throw std::string(sqlite3_errmsg(db));
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

char * DbConnector::temp_icon_file() {
  return temp_icon;
}
  
void DbConnector::fetch_results(std::map<std::pair<int,int>, int> & map) {
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "SELECT * FROM results", -1, &stmt, NULL);
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
  if (rc == SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw std::string("customer not found");
  }
  sqlite3_finalize(stmt);
  return;
}

void DbConnector::update_results(int  i, int  j, int  k) {
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "INSERT INTO results (v1id , v2id,  result) VALUES (?,?, ?) ", -1, &stmt, NULL);
  if (rc != SQLITE_OK)
    throw std::string(sqlite3_errmsg(db));
  rc = sqlite3_bind_int(stmt, 1, i);
  rc = sqlite3_bind_int(stmt, 2, j);
  rc = sqlite3_bind_int(stmt, 3, k);
  if (rc != SQLITE_OK) {                 // really necessary, but recommended
    std::string errmsg(sqlite3_errmsg(db)); // (especially for std::strings) to avoid
    sqlite3_finalize(stmt);            // formatting problems and SQL
    throw errmsg;                      // injection attacks.
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
  
void DbConnector::fetch_trace(int vid, std::vector<int> & trace) {
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "SELECT trace_dat FROM trace_blobs WHERE vid = ? limit 1", -1, &stmt, NULL);
  if (rc != SQLITE_OK)
    throw std::string(sqlite3_errmsg(db));
  rc = sqlite3_bind_int(stmt, 1, vid);    // Using parameters ("?") is not
  if (rc != SQLITE_OK) {                 // really necessary, but recommended
    std::string errmsg(sqlite3_errmsg(db)); // (especially for std::strings) to avoid
    sqlite3_finalize(stmt);            // formatting problems and SQL
    throw errmsg;                      // injection attacks.
  }
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
    std::string errmsg(sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    throw errmsg;
  }
  if (rc == SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw std::string("icon not found");
  }    
  std::string result;
  result.assign(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)));
  boost::char_separator<char> sep("\t");
  boost::tokenizer<boost::char_separator<char>> tokens(result, sep);
  trace.clear();
  for (const std::string& t : tokens)  trace.push_back(atoi(t.c_str()));
  sqlite3_finalize(stmt);
  return;
}

void DbConnector::save_trace(int  vid, std::vector<int> & trace) {
  
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "INSERT INTO trace_blobs (vid , trace_dat) VALUES (? , ?) ", -1, &stmt, NULL);
  if (rc != SQLITE_OK)
    throw std::string(sqlite3_errmsg(db));
  rc = sqlite3_bind_int(stmt, 1, vid);
  std::ifstream input (temp_icon, std::ios::in|std::ios::binary|std::ios::ate);
  std::stringstream stream;
  for(int & i: trace) {
    stream << i << "\t";
    }  
  rc = sqlite3_bind_blob(stmt, 2, stream.str().c_str(), sizeof(stream.str().c_str()),SQLITE_STATIC);                               // Using parameters ("?") is not
  if (rc != SQLITE_OK) {                 // really necessary, but recommended
    std::string errmsg(sqlite3_errmsg(db)); // (especially for std::strings) to avoid
    sqlite3_finalize(stmt);            // formatting problems and SQL
    throw errmsg;                      // injection attacks.
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

