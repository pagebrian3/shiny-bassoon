#include <DbConnector.h>

DbConnector::DbConnector(bfs::path path) {
  bool newFile = true;
  bfs::path db_path = path;
  bfs::path icon_path = path;
  db_path+="/vdb.sql";
  icon_path+="/temp.png";
  temp_icon = icon_path.str().c_str();
  if (bfs::exists(self.db_path)) newFile=false;
  sqlite3_open(db_path,db);
  if (newFile) {
    sqlite3_exec(db,"create table results(v1id integer, v2id integer, result integer)",NULL);
    sqlite3_exec(db,"create table dat_blobs(vid integer primary key, img_dat blob, vdat blob)",NULL);
    sqlite3_exec(db,"create table videos(path text,length real, size real, okflag integer, vdatid integer)",NULL);
  }
}

void DbConnector::save_db_file() {
  sqlite_close(db);
}

vid_file DbConnector::fetch_video(std::string filename){
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "SELECT 1 FROM videos WHERE path = ? limit 1", -1, &stmt, NULL);
  if (rc != SQLITE_OK)
    throw string(sqlite3_errmsg(db));
  rc = sqlite3_bind_text(stmt, 1, filename,-1);    // Using parameters ("?") is not
  if (rc != SQLITE_OK) {                 // really necessary, but recommended
    string errmsg(sqlite3_errmsg(db)); // (especially for strings) to avoid
    sqlite3_finalize(stmt);            // formatting problems and SQL
    throw errmsg;                      // injection attacks.
  }

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
    string errmsg(sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    throw errmsg;
  }
  if (rc == SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw string("customer not found");
  }
    
  char * file = sqlite3_column_text(stmt, 0);
  double length = sqlite3_column_double(stmt,1);
  double size = sqlite3_column_double(stmt,2);
  int flag = sqlite3_column_int(stmt,3);
  int vid = sqlite3_column_int(stmt,4);
  sqlite3_finalize(stmt);
  vid_file result = {file , length, size, flag, vid };
  return result;
}

void DbConnector::fetch_icon(int vid){
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "SELECT img_dat FROM dat_blobs WHERE vid = ? limit 1", -1, &stmt, NULL);
  if (rc != SQLITE_OK)
    throw string(sqlite3_errmsg(db));
  rc = sqlite3_bind_int(stmt, 1, vid,-1);    // Using parameters ("?") is not
  if (rc != SQLITE_OK) {                 // really necessary, but recommended
    string errmsg(sqlite3_errmsg(db)); // (especially for strings) to avoid
    sqlite3_finalize(stmt);            // formatting problems and SQL
    throw errmsg;                      // injection attacks.
  }
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
    string errmsg(sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    throw errmsg;
  }
  if (rc == SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw string("icon not found");
  }    
  unsigned char * result = sqlite3_column_text(stmt, 0);
  std::ofstream output(temp_icon, std::ios::out|std::ios::binary|std::ios::ate);
  output.write(result);
  sqlite3_finalize(stmt);
  output.close();
  return;
}

void DbConnector::save_icon(int & vid) {
  
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "INSERT INTO dat_blobs (vid ,  imgdat) VALUES (? , ?) ", -1, &stmt, NULL);
  if (rc != SQLITE_OK)
    throw string(sqlite3_errmsg(db));
  rc = sqlite3_bind_int(stmt, 1, vid,-1);
  std::ifstream input (temp_icon, std::ios::in|std::ios::binary|std::ios::ate);
  char * memblock;
  int length;
  if (input.is_open())
    {
      std::streampos size = input.tellg();
      memblock = new char [size];
      input.seekg (0, std::ios::beg);
      input.read (memblock, size);
      input.close();
      length = sizeof(*memblock);
    }  
  rc = sqlite3 bind_blob(stmt, 2,length, memblock, SQLITE_STATIC);// Using parameters ("?") is not
  if (rc != SQLITE_OK) {                 // really necessary, but recommended
    string errmsg(sqlite3_errmsg(db)); // (especially for strings) to avoid
    sqlite3_finalize(stmt);            // formatting problems and SQL
    throw errmsg;                      // injection attacks.
  }
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
    string errmsg(sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    throw errmsg;
  }
  if (rc == SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw string("ERROR of some sort");
  }  
  sqlite3_finalize(stmt);
  if (sizeof(*memblock) > 4) delete[] memblock;
  return;
}

bool DbConnector::trace_exists(int vid){
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "SELECT EXISTS(SELECT 1 FROM dat_blobs WHERE vid = ? limit 1)", -1, &stmt, NULL);
  if (rc != SQLITE_OK)
    throw string(sqlite3_errmsg(db));
  rc = sqlite3_bind_int(stmt, 1, vid);    // Using parameters ("?") is not
  if (rc != SQLITE_OK) {                 // really necessary, but recommended
    string errmsg(sqlite3_errmsg(db)); // (especially for strings) to avoid
    sqlite3_finalize(stmt);            // formatting problems and SQL
    throw errmsg;                      // injection attacks.
  }
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
    string errmsg(sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    throw errmsg;
  }
  if (rc == SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw string("trace not found");
  }
  bool result = sqlite3_column_text(stmt, 0);
  sqlite3_finalize(stmt);
  return result;
}

bool DbConnector::video_exists(std::string filename){
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "SELECT EXISTS(SELECT 1 FROM videos WHERE path = ? limit 1)", -1, &stmt, NULL);
  if (rc != SQLITE_OK)
    throw string(sqlite3_errmsg(db));
  rc = sqlite3_bind_text(stmt, 1, filename,-1);    // Using parameters ("?") is not
  if (rc != SQLITE_OK) {                 // really necessary, but recommended
    string errmsg(sqlite3_errmsg(db)); // (especially for strings) to avoid
    sqlite3_finalize(stmt);            // formatting problems and SQL
    throw errmsg;                      // injection attacks.
  }

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
    string errmsg(sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    throw errmsg;
  }
  if (rc == SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw string("video not found");
  }

  bool result = sqlite3_column_text(stmt, 0);

  sqlite3_finalize(stmt);
  return result;
}

int DbConnector::get_last_vid(){
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "SELECT MAX(vid) from dat_blobs", -1, &stmt, NULL);
  if (rc != SQLITE_OK)
    throw string(sqlite3_errmsg(db));
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
    string errmsg(sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    throw errmsg;
  }
  if (rc == SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw string("entry not found");
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
  if (rc != SQLITE_OK) throw string(sqlite3_errmsg(db));
  rc = 0; 
  int a,b,c;
  while((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    a = sqlite3_column_int(stmt, 0);
    b = sqlite3_column_int(stmt, 1);
    c = sqlite3_column_int(stmt, 2);
    map[std::make_pair(a,b)]=c;
  }
  if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
    string errmsg(sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    throw errmsg;
  }
  if (rc == SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw string("customer not found");
  }
  sqlite3_finalize(stmt);
  return;
}

void DbConnector::fetch_trace(int & vid, std::vector<int> & trace) {
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "SELECT v_dat FROM dat_blobs WHERE vid = ? limit 1", -1, &stmt, NULL);
  if (rc != SQLITE_OK)
    throw string(sqlite3_errmsg(db));
  rc = sqlite3_bind_int(stmt, 1, vid,-1);    // Using parameters ("?") is not
  if (rc != SQLITE_OK) {                 // really necessary, but recommended
    string errmsg(sqlite3_errmsg(db)); // (especially for strings) to avoid
    sqlite3_finalize(stmt);            // formatting problems and SQL
    throw errmsg;                      // injection attacks.
  }
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
    string errmsg(sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    throw errmsg;
  }
  if (rc == SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw string("icon not found");
  }    
  unsigned char * result = sqlite3_column_text(stmt, 0);
  std::ofstream output(temp_icon, std::ios::out|std::ios::binary|std::ios::ate);
  output.write(result);
  sqlite3_finalize(stmt);
  output.close();
  return;
}

void DbConnector::save_trace(int & vid, std::vector<int> & trace) {
}

