import matplotlib.pyplot as plt
import tensorflow as tf
from pathlib import Path
from PIL import Image
import base64
import requests
import random
import numpy as np
from io import BytesIO
import shutil
 
ENDPOINT_URL = 'http://0.0.0.0:8080/generate'

def display(display_list):
    plt.figure(figsize=(15, 15))
    title = ['Input Image',  'Output']

    for i in range(len(display_list)):
        plt.subplot(1, len(display_list), i + 1)
        plt.title(title[i])
        plt.imshow(tf.keras.preprocessing.image.array_to_img(display_list[i]))
        plt.axis('off')
    plt.savefig('test_endpoint_output.png')

def infer(input_file): 
    image = open(input_file, 'rb').read()
    # image = np.asarray(tf.image.random_crop(image, (512, 512, image.shape[-1])).numpy())
    # raw_image = Image.fromarray(image)
    # image_data = BytesIO()
    # raw_image.save(image_data, 'png')
    data = { 'image': base64.b64encode(image)}
    response = requests.post(ENDPOINT_URL, json = data)
    response.raise_for_status()
    print(response)
    return response.json()

if __name__ == '__main__':
    images = list(Path('../training/data/input/test').glob('*.png'))
    random.shuffle(images)
    images = images[0:15]
    i = 0
    for im in images:
        print(im)
        out = Image.open(BytesIO(base64.b64decode(infer(im)['image'])))
        shutil.copy(im, f'test_endpoint_input_{i}.png')
        out.save(f'test_endpoint_output_{i}.png')
        i+=1