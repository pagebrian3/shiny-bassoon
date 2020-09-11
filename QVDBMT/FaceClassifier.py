import numpy as np 
import os
import cv2
import sys
from tensorflow.keras import models

extension = 'png'
test_images = []
test_files = []
shape = (int(sys.argv[1]),int(sys.argv[1]))
test_path = sys.argv[2]
threshold = float(sys.argv[4]) 
match_path = test_path+"/match/"
model_path = sys.argv[3]
class_path = Path(model_path).parent+"/classes.txt"
file1 = open( class_path, 'r') 
classes = file1.readlines()
file1.close()
nFiles = 0
for i in range(0,len(classes)):
  classes[i] = classes[i][:-1]  #omit last character
for filename in os.listdir(test_path):
  split = filename.split('.')
  if len(split) > 1 and split[1] == extension:
    test_files.append(filename)
    img = cv2.imread(os.path.join(test_path,filename))
    img = cv2.resize(img,shape)
    test_images.append(img)
    nFiles+=1
if nFiles == 0:
  sys.exit()
test_images = np.array(test_images)
model= models.load_model(model_path)
size = len(test_images)
predict = model.predict(np.array(test_images))
predictions = np.argmax(predict,axis=-1)
for i in range(0,size):
  old_file = test_path+"/"+test_files[i]
  if(predict[i][predictions[i]] > threshold):   
    new_file = match_path+"/"+classes[predictions[i]]+"_"+str(predict[i][predictions[i]])+"_"+test_files[i]
    os.rename(old_file,new_file)
  else:
    new_file = match_path+"/unknown_"+str(predict[i][predictions[i]])+"_"+test_files[i]
    os.rename(old_file,new_file)
