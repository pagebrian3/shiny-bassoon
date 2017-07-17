#include "opencv2/videoio.hpp"
#include "opencv2/video.hpp"
#include "root/TFile.h"
#include "root/TTree.h"
#include "root/TObject.h"
#include "root/TKey.h"
#include <boost/filesystem.hpp>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <memory>
#include <cassert>
#include <vector>
#include <string>
#include <algorithm>
#include <set>
#include <iostream>
#include <chrono>

bool load_video(std::string videoFile, std::vector<int> *array , std::vector<float> *times, int & size);
						 
int main(int argc, char* argv[])
{
  std::set<std::string> extensions{".mp4",".wmv",".mov",".rm",".m4v",".flv",".avi",".qt",".mpg",".mpeg",".mpv",".3gp"};
  std::vector<std::string> vid_files;
  std::map<std::string,int> vids_in_file;
  if(boost::filesystem::exists("mytestfile.root")) {
    TFile *f = new TFile("mytestfile.root", "READ");
    TIter next1(f->GetListOfKeys());
    TKey * key;
    TTree * tree;
    std::string name;
    while(key = (TKey*)next1())  {
      TTree *tree = (TTree*)key->ReadObj();
      name= tree->GetName();
      vids_in_file[name]=1;
    }
    delete f;
  }
  TFile *f = new TFile("mytestfile.root" ,"UPDATE");
  std::string extension;
  boost::filesystem::path p(argv[1]);
  for (boost::filesystem::directory_entry & x : boost::filesystem::recursive_directory_iterator(p))
    {
      if(vids_in_file[x.path().generic_string()]) continue;
      extension = x.path().extension().generic_string();
      std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
      if(extensions.count(extension)) vid_files.push_back(x.path().string());
    }
  int max_size = 450000;
  auto array = new std::vector<int>(12*max_size);
  auto time_array = new std::vector<float>(max_size);
  for(int i=0; i < vid_files.size(); i++)
    {
      std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
      TTree * tree = new TTree(vid_files[i].c_str(),"");
      std::cout <<i <<"/"<<vid_files.size()<< " BLAH "<<vid_files[i] << std::endl;
      int size;
      load_video(vid_files[i],array,time_array,size);
      int vSize=12*size;
      //int dataHolder[vSize];
      std::vector<int> * dataHolder = new std::vector<int>(vSize);
      //float timeHolder[size];
      std::vector<float> * timeHolder = new std::vector<float>(size);
      cv::Scalar intensity;
      for(int h = 0; h <size; h++){
	(*timeHolder)[h]=(*time_array)[h];
	for(int j = 0; j < 12; j++) 
	  (*dataHolder)[h*12+j]=(*array)[h*12+j];	    
      }
      TBranch * branch = tree->Branch("vdat",&(*dataHolder)[0],TString::Format("vdat[%i]/I",vSize));
      TBranch * branch1 = tree->Branch("tdat",&(*timeHolder)[0],TString::Format("tdat[%i]/F",size));
      TBranch * branch2 = tree->Branch("dSize",&size,"dSize/I");
      tree->Fill();
      tree->Write();
      delete tree;
      delete dataHolder;
      delete timeHolder;
      std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( t2 - t1 ).count();
      std::cout << "Time: " << duration << std::endl;
    } 
  f->Write();
  f->Close();
  return 0;
}   

bool load_video(std::string videoFile, std::vector<int> *array , std::vector<float> *times,int & size)
{
  cv::Mat frame;
  cv::Mat a;
  cv::VideoCapture vCapt(videoFile,cv::CAP_FFMPEG);
  float time_pos;
  size = 0;
  //int frameTime = 0;
  //int convertTime=0;
  while(true)
    {
      cv::Mat frame;
      //std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
      
      vCapt >> frame;
      if (frame.empty()) break;
      time_pos = vCapt.get(cv::CAP_PROP_POS_MSEC);
      //std::cout << "a " <<time_pos << std::endl;
      //std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
      cv::resize(frame,a,cv::Size(2,2),0,0,cv::INTER_AREA);
      //std::chrono::high_resolution_clock::time_point t3 = std::chrono::high_resolution_clock::now();
      //convertTime+=std::chrono::duration_cast<std::chrono::microseconds>( t3 - t2 ).count();
      //frameTime+=std::chrono::duration_cast<std::chrono::microseconds>( t2 - t1 ).count();
      for(int i = 0; i < 12; i++) (*array)[12*size+i]=a.data[i];
  (*times)[size]=time_pos;
  //if(array.size() > 0)  for(int i = 0; i < 13; i++) std::cout <<(int)a.data[i] <<" ";
  //std::cout<< std::endl;
  /*
     int height = frame.rows;
     int width = frame.cols;
     double weight = 1.0/(4.0*(double)height*(double)width);
     int lHeight = height/2;
     int lWidth = width/2; 	
     if(width%2!=0) lWidth++;
     if(height%2!=0) lHeight++;
     std::vector<int> temp(12);
     for(int i = 0; i < 2; i++)
     {
     temp[i].resize(2);
     for(int j = 0; j < 2; j++) temp[i][j].resize(3);
     }
     for(int i = 0; i <lWidth; i++)
     for(int j = 0; j <lHeight; j++)
     for(int k = 0; k <3; k++)
     {
     temp[0][0][k]+=weight*frame.at<cv::Vec3b>(i,j).val[k];
     temp[0][1][k]+=weight*frame.at<cv::Vec3b>(i,j+width/2).val[k];
     temp[1][0][k]+=weight*frame.at<cv::Vec3b>(i+width/2,j).val[k];
     temp[1][1][k]+=weight*frame.at<cv::Vec3b>(i+width/2,j+width/2).val[k];
     }
     if(width%2!=0)
     for(int i = 0; i < lHeight; i++)
     for(int k =0; k < 3; k++)
     {
     temp[0][0][k]-=0.5*weight*frame.at<cv::Vec3b>(height/2,i).val[k];
     temp[0][1][k]-=0.5*weight*frame.at<cv::Vec3b>(height/2,i+width/2).val[k];
     temp[1][0][k]-=0.5*weight*frame.at<cv::Vec3b>(height/2,i).val[k];
     temp[1][1][k]-=0.5*weight*frame.at<cv::Vec3b>(height/2,i+width/2).val[k];
     }
     if(height%2!=0)
     for(int i = 0; i < lWidth; i++)
     for(int k =0; k < 3; k++)
     {
     temp[0][0][k]-=0.5*weight*frame.at<cv::Vec3b>(i,width/2).val[k];
     temp[0][1][k]-=0.5*weight*frame.at<cv::Vec3b>(i, width/2).val[k];
     temp[1][0][k]-=0.5*weight*frame.at<cv::Vec3b>(i+width/2,width/2).val[k];
     temp[1][1][k]-=0.5*weight*frame.at<cv::Vec3b>(i+width/2,width/2).val[k];
     }
     if(width%2!=0 && height%2!=0)
     {
     for(int k = 0; k < 3; k++){
     double val=frame.at<cv::Vec3b>(height/2,width/2).val[k];
     temp[0][0][k]+=0.25*weight*val;
     temp[0][1][k]+=0.25*weight*val;
     temp[1][0][k]+=0.25*weight*val;
     temp[1][1][k]+=0.25*weight*val;
     }
     }
     //for(int i = 0; i < 2; i++) for(int j = 0; j < 2; j++) for(int k = 0; k < 3; k++) a.at<cv::Vec3b>(i,j).val[k]=temp[i][j][k];
     for(int i = 0; 
  */
  size++;                    
    }
  //std::cout <<"Ft Ct " << frameTime <<" "<<convertTime << std::endl;
return true;

}



