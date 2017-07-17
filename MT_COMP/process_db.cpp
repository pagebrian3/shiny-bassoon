#include "root/TFile.h"
#include "root/TTreeReader.h"
#include "root/TTreeReaderValue.h"
#include "root/TTreeReaderArray.h"
#include "root/TObject.h"
#include "root/TClass.h"
#include "root/TROOT.h"
#include "root/TKey.h"
#include <vector>
#include <string>
#include <iostream>
#include <chrono>
#include <boost/filesystem.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include "threadpool.hpp"

#define FPS 5.0
#define WINDOW 5.0
#define TOLERANCE 3000.0
#define START 0

//boost::mutex mtx_;

void load_data(TTree *tree,std::vector<float> &tData,std::vector<int> &vData);

void compare_videos(std::vector<float> t1, std::vector<int> v1,std::string name1, std::vector<float> t2, std::vector<int> v2, std::string name2); 

int get_next(float time, std::vector<float> &tDat, int & lastPos);

std::map<std::pair<std::string,std::string>,int>  videoMap;

int main(int argc, char * argv[]) {
  int MAX_THREADS = std::atoi(argv[1]);
  std::chrono::high_resolution_clock::time_point time1 = std::chrono::high_resolution_clock::now();
  auto threadPool= new boost::threadpool::pool(MAX_THREADS);
  TFile *fMap;
  std::string file_name1;
  std::string file_name2;
  int result;
  if(boost::filesystem::exists("testMap.root")) {
    fMap = new TFile("testMap.root","READ");
    TTreeReader tReader("video_map", fMap);
    TTreeReaderArray<char> fName1(tReader,"fname1");
    TTreeReaderArray<char> fName2(tReader,"fname2");
    TTreeReaderValue<int> fResult(tReader,"result");
    while(tReader.Next()) {
      for(int i = 0; fName1[i]!='\n'; i++) {
	file_name1.push_back(fName1[i]);
      }
      for(int i = 0; fName2[i]!='\n'; i++) {
	file_name2.push_back(fName2[i]);
      }
      result = *fResult;
      videoMap[std::make_pair(file_name1,file_name2)]=result;
      file_name1.clear();
      file_name2.clear();
    }
    delete fMap;
  }
  TFile *f = new TFile("mytestfile.root", "READ");
  TIter next1(f->GetListOfKeys());
  int size = 0;
  TKey * key;
  std::vector<int> v1,v2;
  std::vector<float> t1,t2;
  while(key = (TKey*)next1()) size++;
  int counter = 0;
  for(int i = START; i < size-1; i++){
    next1 = f->GetListOfKeys();
    counter = 0;
    while(counter <= i) {
      key = (TKey*)next1();
      counter++;
    }
    TTree *tree1 = (TTree*)key->ReadObj();
    std::string name1 = tree1->GetName();
    load_data(tree1, t1,v1);
    for(int j = i+1; j < size; j++){
      next1 = f->GetListOfKeys();
      counter = 0;
      while(counter <= j) {
	key = (TKey*)next1();
	counter++;
      }
      TTree *tree2 = (TTree*)key->ReadObj();
      std::string name2 = tree2->GetName();
      std::cout << name1 <<" "<<name2 << std::endl;
      if(videoMap[std::make_pair(name1,name2)]!=0) continue;
      std::cout << "HERE" << std::endl;
      load_data(tree2, t2,v2);
      threadPool->schedule(boost::bind(compare_videos, t1,v1,name1,t2,v2,name2));
      /*{
	videoMap[make_pair(std::string(name1),std::string(name2))]=2;
	std::cout << "MATCH: " <<name1 << " "<<name2<<std::endl;
	}*/
      //else videoMap[make_pair(std::string(name1),std::string(name2))]=1;
    }
  }
  delete threadPool;
  fMap = new TFile("testMap.root","RECREATE");
  TTree * mapTree = new TTree("video_map","video_map");
  char f1[1000];
  char f2[1000];
  TBranch *f1Branch = mapTree->Branch("fname1",&f1,"fname1/C");
  TBranch *f2Branch = mapTree->Branch("fname2",&f2,"fname2/C");
  TBranch *rBranch = mapTree->Branch("result",&result,"result/I");
  auto map_iter=videoMap.begin();
  while(map_iter!=videoMap.end()) {
    strcpy(f1,((*map_iter).first.first+'\n').c_str());
    strcpy(f2,((*map_iter).first.second+'\n').c_str());
    result = (*map_iter).second;
    mapTree->Fill();
    map_iter++;
  }
  fMap->Write();
  std::chrono::high_resolution_clock::time_point time2 = std::chrono::high_resolution_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::microseconds>( time2 - time1 ).count();
      std::cout << "Time: " << duration << std::endl;
  return 0;
}

void load_data(TTree *tree,std::vector<float> &tData,std::vector<int> &vData)
{
  tData.clear();
  vData.clear();
  //boost::lock_guard<boost::mutex> guard(mtx_);
  TTreeReader tReader(tree);
  TTreeReaderArray<float> timeData(tReader,"tdat");
  TTreeReaderArray<int> videoData(tReader,"vdat");
  tReader.Next();
  for(const float& t: timeData) tData.push_back(t);
  for(const int& v: videoData) vData.push_back(v);
}

void compare_videos(std::vector<float> t1, std::vector<int> v1,std::string name1, std::vector<float> t2, std::vector<int> v2, std::string name2)
{
  std::cout << "thread start" << std::endl;
  if(t1.size() < 10 || t2.size() < 10){
    videoMap[make_pair(name1,name2)]=1;
    return;
  }
  float t0 = (t1[0]+t2[0])/2.0;
  float end_time1 = t1.back();
  float end_time2 = t2.back();
  std::vector<float> res(3);
  int tPos1, tPos2;
  int t1Bins = end_time1/(1000.0*WINDOW);
  int t2Bins = end_time2*FPS/1000.0;
  for(float t1delta = t0; t1delta < end_time1-WINDOW*1000.0; t1delta+=1000.0*WINDOW)
    {	
      for(float t2delta = t0; t2delta < end_time2-WINDOW*1000.0; t2delta+=1000.0/FPS)
	{
	  tPos1=0;
	  tPos2=0;
	  res[0]=res[1]=res[2]=0;
	  for(float tWin = 0; tWin < WINDOW*1000.0; tWin+=1000.0/FPS)
	    {
	      tPos1 = get_next(t1delta+tWin,t1,tPos1);
	      tPos2 = get_next(t2delta+tWin,t2,tPos2);
	      if(tPos1 == -2 || tPos2 == -2) std::cout << "Danger" <<std::endl;
	      for(int i = 0; i < 4; i++)
		for(int j = 0; j < 3; j++)
		  res[j]+=pow(v1[12*tPos1+4*i+j]-v2[12*tPos2+4*i+j],2.0);	
	      if(res[0] > TOLERANCE ||
		 res[1] > TOLERANCE ||
		 res[2] > TOLERANCE) break;
	      
	    }
	  if(res[0] < TOLERANCE &&
	     res[1] < TOLERANCE &&
	     res[2] < TOLERANCE) {
	    videoMap[make_pair(name1, name2)]=2;
	    //std::cout << "Match" << std::endl;
	    return;
	  }
	}
    }
  //boost::lock_guard<boost::mutex> guard(mtx_);
  videoMap[make_pair(name1,name2)]=1;
  std::cout << "Thread End" << std::endl;
  return;
}

int get_next(float time, std::vector<float> &tDat, int &lastPos) {
  if(time > tDat.back()) return -2;
  int i = lastPos;
  while(tDat[i] < time) i++;
  if(tDat[i]-time < time-tDat[i-1]) return i;
  else return i-1;
}


   

