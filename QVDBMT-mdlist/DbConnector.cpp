#include <DbConnector.h>
#include <VidFile.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <sqlite3.h>
#include <iostream>

DbConnector::DbConnector(std::filesystem::path & appPath,std::filesystem::path & tempPath) {
  db_path = appPath;
  std::filesystem::path tmpPath = tempPath;
  db_path+="vdb.db";
  db_tmp = tmpPath;
  db_tmp+="vdb.db";
  newDB = true;
  if (std::filesystem::exists(db_path)) {
    newDB=false;
    std::filesystem::copy_file(db_path,db_tmp,std::filesystem::copy_options::overwrite_existing);
  }  
  sqlite3_open(db_tmp.c_str(),&db);
  if (newDB) {
    sqlite3_exec(db,"create table videos(vid integer primary key not null, path text,crop text, length double, size integer, okflag integer, rotate integer, height integer, width integer)", NULL,NULL, NULL);
    sqlite3_exec(db,"create table results(v1id integer, v2id integer, test integer, result integer, details text)",NULL, NULL, NULL);
    sqlite3_exec(db,"create table config(cfg_label text primary key,cfg_type integer,cfg_int integer,cfg_float double,cfg_str text)",NULL,NULL,NULL);
    sqlite3_exec(db,"create table md_types(md_type_index integer primary key not null, md_type_label text unique)",NULL,NULL,NULL);
    sqlite3_exec(db,"create table md_index(md_idx integer primary key not null, md_type integer not null, md_label text unique)",NULL,NULL,NULL);
    sqlite3_exec(db,"create table file_mdx(vid integer primary key, tagids text,FOREIGN KEY(vid) REFERENCES videos(vid))",NULL,NULL,NULL);
  }
}

void DbConnector::save_db_file() {
  sqlite3_close(db);
  std::filesystem::copy_file(db_tmp,db_path,std::filesystem::copy_options::overwrite_existing);
  std::filesystem::remove(db_tmp);
}

VidFile * DbConnector::fetch_video(std::filesystem::path & filename){
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
  std::vector<int> cropArray(4);
  if(crop.length() > 0) {
    std::vector<std::string> split_vec;
    boost::algorithm::split(split_vec, crop, boost::is_any_of(","));
    for(int i = 0; i < 4; i++) cropArray[i]=std::atoi(split_vec[i].c_str());
  }
  float length = sqlite3_column_double(stmt,1);
  int size = sqlite3_column_int(stmt,2);
  int flag = sqlite3_column_int(stmt,3);
  int rotate = sqlite3_column_int(stmt,4);
  int vid = sqlite3_column_int(stmt,5);
  int height = sqlite3_column_int(stmt,6);
  int width = sqlite3_column_int(stmt,7);
  sqlite3_finalize(stmt); 
  VidFile * result = new VidFile(filename, length, size, flag, vid, cropArray, rotate, height, width);
  return result;
}

void DbConnector::save_video(VidFile* a) {
  std::string crop_holder("");
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "INSERT INTO videos(path,crop,length,size,okflag,rotate,width,height) VALUES (?,?,?,?,?,?,?,?) ", -1, &stmt, NULL);
  if (rc != SQLITE_OK) throw std::string(sqlite3_errmsg(db));
  if(a->crop[0]+a->crop[1]+a->crop[2]+a->crop[3] > 0)  {
     std::stringstream ss;
    int counter=0;
    while(counter < 3) {
      ss << a->crop[counter] << ",";
      counter++;
    }
    ss << a->crop[counter]<<std::endl;
    crop_holder=ss.str();
  }
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

bool DbConnector::video_exists(std::filesystem::path & filename){
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
  
void DbConnector::fetch_results(std::map<std::tuple<int,int,int>, std::pair<int,std::string>> & map) {
  /*sqlite3_stmt *cstmt;
  int rc = sqlite3_prepare_v2(db,"SELECT Count(*) FROM results", -1, &cstmt, NULL);
  sqlite3_step(cstmt);
  if (sqlite3_column_int(cstmt, 0) == 0) {
    sqlite3_finalize(cstmt);
    return;
  }
  sqlite3_finalize(cstmt);*/
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "SELECT * FROM results", -1, &stmt, NULL);
  if (rc != SQLITE_OK) throw std::string(sqlite3_errmsg(db));
  int a,b,c,d;
  while((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    a = sqlite3_column_int(stmt, 0);
    b = sqlite3_column_int(stmt, 1);
    c = sqlite3_column_int(stmt, 2);
    d = sqlite3_column_int(stmt, 3);
    std::string details(reinterpret_cast<const char *>(sqlite3_column_text(stmt,4)));
    map[std::make_tuple(a,b,c)]=std::make_pair(d,details);
  }
  if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
    std::string errmsg(sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    throw errmsg;
  }
  sqlite3_finalize(stmt);
  return;
}

void DbConnector::update_results(int  i, int  j, int  k, int l, std::string details) {
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "INSERT or REPLACE INTO results (v1id, v2id, test, result, details) VALUES (?,?,?,?,?) ", -1, &stmt, NULL);
  if (rc != SQLITE_OK) throw std::string(sqlite3_errmsg(db));
  rc = sqlite3_bind_int(stmt, 1, i);
  rc = sqlite3_bind_int(stmt, 2, j);
  rc = sqlite3_bind_int(stmt, 3, k);
  rc = sqlite3_bind_int(stmt, 4, l);
  rc = sqlite3_bind_text(stmt, 5, details.c_str(),-1, NULL);
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

std::vector<int> DbConnector::cleanup(std::filesystem::path & dir, std::vector<std::filesystem::path> & files){
  if(newDB) return std::vector<int>();
  sqlite3_stmt *stmt;
  std::vector<int> orphans;
  std::stringstream ss;
  ss << "SELECT path,vid FROM videos WHERE path LIKE '" << dir.c_str() <<"%'" << std::endl;
  int rc = sqlite3_prepare_v2(db,ss.str().c_str(), -1, &stmt, NULL);
  if (rc != SQLITE_OK) throw std::string(sqlite3_errmsg(db));
  while((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    std::filesystem::path path1(reinterpret_cast<const char *>(sqlite3_column_text(stmt,0)));
    std::filesystem::path relative = std::filesystem::relative(dir,path1);
    if(relative.string().find_first_of('/') != std::string::npos) continue;
    auto result = std::find(files.begin(),files.end(),path1);
    if(result == files.end()) orphans.push_back(sqlite3_column_int(stmt,1));
  }
  sqlite3_finalize(stmt);
  for(auto & a: orphans) {
    sqlite3_prepare_v2(db, "DELETE FROM file_mdx WHERE vid = ?", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, a); 
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_prepare_v2(db, "DELETE FROM results WHERE v1id = ? OR v2id = ?", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, a); 
    sqlite3_bind_int(stmt, 2, a); 
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_prepare_v2(db, "DELETE FROM videos WHERE vid = ?", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, a); 
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
  }  
  return orphans;
}

std::vector<std::pair<std::string,boost::variant<int,float,std::string>>> DbConnector::fetch_config() {
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

void DbConnector::load_fileVIDs(std::map<int,std::filesystem::path> & fvMap) {
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "SELECT vid, path FROM videos", -1, &stmt, NULL);
  if (rc != SQLITE_OK) throw std::string(sqlite3_errmsg(db));
  while((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    int vid = sqlite3_column_int(stmt,0);
    std::filesystem::path fPath(reinterpret_cast<const char *>(sqlite3_column_text(stmt,1)));
    fvMap[vid]=fPath;
  }
  sqlite3_finalize(stmt);
  return;
}

void DbConnector::load_metadata_for_files(std::vector<int> & vids,std::map<int,std::set<int > > & fileMetadata) {
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
    std::set<int> tag_ids;
    for(auto & s: split_vec) tag_ids.insert(std::atoi(s.c_str()));
    fileMetadata[vid]=tag_ids;
    sqlite3_finalize(stmt);
    //std::cout << *(tag_ids.begin()) << " " << vid << std::endl;
  }
  return;
}

void DbConnector::save_metadata(std::map<int,std::set<int> > & file_metadata,std::map<int,std::pair<int,std::string> > & labelLookup, boost::bimap<int, std::string> & typeLookup) {
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
    
    sqlite3_stmt *stmt;
    std::stringstream ss;
    int rc = 0;
    if(a.second.size() == 0) {
      rc = sqlite3_prepare_v2(db, "DELETE from file_mdx where VID=?", -1, &stmt, NULL);
    if (rc != SQLITE_OK) throw std::string(sqlite3_errmsg(db));
    rc = sqlite3_bind_int(stmt, 1, a.first);
    }
    else {
      uint counter=0;
      auto setIter = a.second.begin();
      while(counter < a.second.size()-1) {
	ss << *setIter << ",";
	setIter++;
	counter++;
      }
      ss << *setIter;   
      rc = sqlite3_prepare_v2(db, "INSERT or REPLACE INTO  file_mdx(vid, tagids) VALUES (?,?) ", -1, &stmt, NULL);
      if (rc != SQLITE_OK) throw std::string(sqlite3_errmsg(db));
      rc = sqlite3_bind_int(stmt, 1, a.first);
      rc = sqlite3_bind_text(stmt, 2, ss.str().c_str(),-1, NULL);
    }
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

int DbConnector::fileVid(std::filesystem::path & fileName) {
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "SELECT vid FROM videos WHERE path = ? limit 1", -1, &stmt, NULL);
  if (rc != SQLITE_OK) throw std::string(sqlite3_errmsg(db));
  rc = sqlite3_bind_text(stmt, 1, fileName.c_str(),-1, NULL);    
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
  int result = sqlite3_column_int(stmt, 0);
  sqlite3_finalize(stmt);
  return result;
}
