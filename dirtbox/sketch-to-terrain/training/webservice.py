import os
import base64

from webservice.model import TerrainModel
from gan.custom_layers import load_model
import gan.model as GAN
from flask import jsonify 
from flask import Flask, request
from PIL import Image
import numpy as np
from io import BytesIO
import tensorflow as tf
import cv2

APP_ROOT = os.getenv('APP_ROOT', '/sketch-to-terrain')
HOST = os.getenv('HOST', 'localhost')
PORT_NUMBER = os.getenv('PORT_NUMBER', '8080')
app = Flask(__name__)

gpus = tf.config.experimental.list_physical_devices('GPU')
if gpus:
    try:
        # Currently, memory growth needs to be the same across GPUs
        for gpu in gpus:
            tf.config.experimental.set_memory_growth(gpu, True)
        logical_gpus = tf.config.experimental.list_logical_devices('GPU')
        print(len(gpus), "Physical GPUs,", len(logical_gpus), "Logical GPUs")
    except RuntimeError as e:
        # Memory growth must be set before GPUs have been initialized
        print(e)
else:
    print('NO REGISTERED GPUS!!!!!!!!!!!')
    exit(-1)

@app.route('/generate', methods=["POST"]) 
def infer():
    data = request.json
    image_data = base64.b64decode(data['image'])
    image = np.asarray(Image.open(BytesIO(image_data)).convert('RGB'))
    # Image.open(BytesIO(image_data)).convert('RGB').save("dump.png")
    # cv2.resize(image, (512, 512, 3), )
    
    terrain_generator = load_model(f'terrain_generator48.h5')

    source = np.array(image, dtype=np.float)
    source = (source - 127.0) / 127.0
    shape = source.shape
    source = np.reshape(source, [1, shape[0], shape[1], shape[2]])
    w_noise = GAN.latent_noise(1, terrain_generator.input_shape[1][1])

    predicted = terrain_generator.predict([source, w_noise])
    predicted = predicted * 0.5 + 0.5
    mint = np.min(predicted)
    predicted = (predicted - mint) / (np.max(predicted) - mint)
    im = np.uint8(predicted[0, ...] * 255)
    result = im
    
    # result = t_model.generate(image)[0]




    shape = result.shape
    result = np.tile(np.reshape(result, shape[0:2])[:, :, None], [1, 1, 3])
    print(result.shape)
    # result = cv2.GaussianBlur(result, (9, 9), cv2.BORDER_DEFAULT)
    image = Image.fromarray(result)
    img_byte_arr = BytesIO()
    image.save(img_byte_arr, format='PNG')
    out = {'image': base64.b64encode(img_byte_arr.getvalue())}
    return out

@app.errorhandler(Exception)
def handle_exception(e):
    return jsonify(stackTrace=traceback.format_exc())

if __name__ == '__main__':
    
    app.run(host=HOST, port=PORT_NUMBER)