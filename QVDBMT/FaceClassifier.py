# Split this into train and test
import numpy as np 
import os
import cv2
import sys
from tensorflow.keras import models

extension = 'png'
test_images = []
test_labels = []
test_files = []
test_path = sys.argv[2]
shape = (int(sys.argv[1]),int(sys.argv[1]))
for filename in os.listdir(test_path):
    if filename.split('.')[1] == extension:
        test_files.append(filename)
        img = cv2.imread(os.path.join(test_path,filename))
        img = cv2.resize(img,shape)
        test_images.append(img)        
test_images = np.array(test_images)
model= models.load_model("model.keras")
size = len(test_images)
f = open("output.txt", "a")
predict = model.predict(np.array(test_images))
predictions = np.argmax(predict,axis=-1)
for i in range(0,size):
    f.write(test_files[i] + " " + str(predictions[i]) + " " + str(predict[i][predictions[i]]) + "\n")
f.close()
