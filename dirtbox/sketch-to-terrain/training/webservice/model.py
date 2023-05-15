import tensorflow as tf
import numpy as np
from gan import custom_layers
from gan.model import latent_noise

class TerrainModel:
    def __init__(self, model_path): 
        self.saved_path = model_path
        self.model = custom_layers.load_model(self.saved_path)
        # self.w_noise = latent_noise(1, 32)

    def generate(self, input_image):
        print(input_image.shape)
        w_noise = latent_noise(1, self.model.input_shape[1][1])
        tensor_image = tf.convert_to_tensor(input_image, dtype=tf.float32)
        tensor_image = (tensor_image - 127.0) / 127.0
        shape = tensor_image.shape
        tensor_image = tf.reshape(tensor_image,[1, shape[0], shape[1], shape[2]])
        print(tensor_image.shape)
        predicted = self.model.predict([tensor_image, w_noise], batch_size=1)
        predicted = predicted * 0.5 + 0.5
        mint = np.min(predicted)
        predicted = (predicted - mint) / (np.max(predicted) - mint)
        predicted = np.clip(predicted, 0, 1)
        predicted = ((predicted * 255)).astype(np.uint8)
        return predicted