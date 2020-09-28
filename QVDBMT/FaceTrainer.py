#!/bin/python
import numpy as np 
import pandas as pd 
import os
import cv2
import sys
from keras.utils import to_categorical
from keras.layers import Dense,Conv2D,Flatten,MaxPool2D,Dropout
from keras.models import Sequential
from sklearn.model_selection import train_test_split

np.random.seed(1)
train_images = []       
train_labels = []
inputShape = (int(sys.argv[1]),int(sys.argv[1]),3,)
train_path = sys.argv[2]
extension = 'png'
for filename in os.listdir(train_path):
    if filename.split('.')[1] == extension:
        img = cv2.imread(os.path.join(train_path,filename))
        train_labels.append(filename.split('_')[0])
        train_images.append(img)
classes = set(train_labels)
f = open("classes.txt", "w")
for class_name in classes:
    f.write(class_name+'\n')
f.close()
train_labels = pd.get_dummies(train_labels).values
# Converting train_images to array
train_images = np.array(train_images)
# Splitting Training data into train and validation dataset
x_train,x_val,y_train,y_val = train_test_split(train_images,train_labels,random_state=1)
model= Sequential([ Conv2D(kernel_size=(3,3), filters=32, activation='tanh', input_shape=inputShape) , Conv2D(filters=30,kernel_size = (3,3),activation='tanh') , MaxPool2D(2,2) , Conv2D(filters=30,kernel_size = (3,3),activation='tanh') , MaxPool2D(2,2) , Conv2D(filters=30,kernel_size = (3,3),activation='tanh') , Flatten() , Dense(20,activation='relu') , Dense(15,activation='relu') , Dense(len(classes),activation = 'softmax') ])
model.compile(
              loss='categorical_crossentropy', 
              metrics=['acc'],
              optimizer='adam'
             )
model.fit(x_train,y_train,epochs=50,batch_size=50,validation_data=(x_val,y_val))
model.save('model.keras')



