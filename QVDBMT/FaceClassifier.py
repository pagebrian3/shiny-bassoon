# Split this into train and test
import numpy as np 
import pandas as pd 
import os
import matplotlib.pyplot as plt
import cv2
import sys
from keras.utils import to_categorical
from keras.layers import Dense,Conv2D,Flatten,MaxPool2D,Dropout
from keras.models import Sequential
from tensorflow.keras import models

np.random.seed(1)
extension = 'png'
test_images = []
test_labels = []
test_files = []
test_path = sys.argv[3]
shape = (int(sys.argv[1]),int(sys.argv[2]))

for filename in os.listdir(test_path):
    if filename.split('.')[1] == extension:
        test_files.append(filename)
        img = cv2.imread(os.path.join(test_path,filename))
        
        # Spliting file names and storing the labels for image in list
        test_labels.append(filename.split('_')[0])
        
        # Resize all images to a specific shape
        img = cv2.resize(img,shape)
        
        test_images.append(img)
        
# Converting test_images to array
test_images = np.array(test_images)

# Creating a Sequential model
model= models.load_model("model.keras");

# Testing predictions and the actual label
output = {}
count = 0
for label in test_labels:
    output[count]=label
    count+=1
size = len(test_images)
f = open("output.txt", "a")
for i in range(0,size):
    checkImage = test_images #[i:i+1]
    predict = np.argmax(model.predict(np.array(checkImage)),axis=-1)   
    f.write(test_files[i] +" " +output[predict[i]] + "\n")
f.close()
