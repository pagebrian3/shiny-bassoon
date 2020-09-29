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

from sklearn.model_selection import train_test_split
#would be nice to re-enable at some point, but right now this mixes eager and non-eager execution, which is a "no no"
from tensorflow.python.framework.ops import disable_eager_execution

disable_eager_execution()

np.random.seed(1)

# -> appending images in a list 'train_images'
# -> appending labels in a list 'train_labels'

train_images = []       
train_labels = []
train_path = sys.argv[2]
extension = 'png'

for filename in os.listdir(train_path):
    if filename.split('.')[1] == extension:
        img = cv2.imread(os.path.join(train_path,filename))
        
        # Spliting file names and storing the labels for image in list
        train_labels.append(filename.split('_')[0])
        
        train_images.append(img)

# Converting labels into One Hot encoded sparse matrix
train_labels = pd.get_dummies(train_labels).values

# Converting train_images to array
train_images = np.asarray(train_images)

# Splitting Training data into train and validation dataset
x_train,x_val,y_train,y_val = train_test_split(train_images,train_labels,random_state=1)

# Processing testing data
# -> appending images in a list 'test_images'
# -> appending labels in a list 'test_labels'
# The test data contains labels as well also we are appending it to a list but we are'nt going to use it while training.

test_images = []
test_labels = []
test_path = sys.argv[3]

for filename in os.listdir(test_path):
    if filename.split('.')[1] == extension:
        img = cv2.imread(os.path.join(test_path,filename))
        
        # Spliting file names and storing the labels for image in list
        test_labels.append(filename.split('_')[0])
        
        test_images.append(img)
        
# Converting test_images to array
test_images = np.array(test_images)

# Visualizing Training data
print(train_labels[0])
plt.imshow(train_images[0])

# Visualizing Training data
print(train_labels[4])
plt.imshow(train_images[4])

# Creating a Sequential model
model= Sequential()
model.add(Conv2D(kernel_size=(3,3), filters=32, activation='tanh', input_shape=(int(sys.argv[1]),int(sys.argv[1]),3,)))
model.add(Conv2D(filters=30,kernel_size = (3,3),activation='tanh'))
model.add(MaxPool2D(2,2))
model.add(Conv2D(filters=30,kernel_size = (3,3),activation='tanh'))
model.add(MaxPool2D(2,2))
model.add(Conv2D(filters=30,kernel_size = (3,3),activation='tanh'))

model.add(Flatten())

model.add(Dense(20,activation='relu'))
model.add(Dense(15,activation='relu'))
model.add(Dense(2,activation = 'softmax'))
    
model.compile(
              loss='categorical_crossentropy', 
              metrics=['acc'],
              optimizer='adam'
             )

model.summary()

history = model.fit(x_train,y_train,epochs=50,batch_size=50,validation_data=(x_val,y_val))

# summarize history for accuracy
plt.plot(history.history['acc'])
plt.plot(history.history['val_acc'])
plt.title('model accuracy')
plt.ylabel('accuracy')
plt.xlabel('epoch')
plt.legend(['train', 'test'], loc='upper left')
plt.show()

# summarize history for loss
plt.plot(history.history['loss'])
plt.plot(history.history['val_loss'])
plt.title('model loss')
plt.ylabel('loss')
plt.xlabel('epoch')
plt.legend(['train', 'test'], loc='upper left')
plt.show()

# Evaluating model on validation data
evaluate = model.evaluate(x_val,y_val)
print(evaluate)

# Testing predictions and the actual label
checkImage = test_images[0:1]
checklabel = test_labels[0:1]

predict = model.predict(np.asarray(checkImage))

output = {}
count = 0
for label in train_labels:
    output[count]=label
    count+=1
print("Actual :- ",checklabel)
print("Predicted :- ",output[np.argmax(predict)])
