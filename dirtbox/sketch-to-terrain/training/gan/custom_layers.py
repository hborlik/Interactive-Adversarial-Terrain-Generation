import tensorflow as tf
import numpy as np

import six

from tensorflow.keras import layers
from tensorflow.keras.constraints import Constraint
from tensorflow.python.keras.utils import conv_utils
from tensorflow.python.keras import backend
from tensorflow.python.ops import array_ops
from tensorflow.python.ops import nn
from tensorflow.python.ops import nn_ops
from tensorflow.python.keras.utils import tf_utils

from tensorflow.python.keras import constraints
from tensorflow.python.keras import initializers
from tensorflow.python.keras import regularizers
from tensorflow.python.util.tf_export import keras_export

from keras_contrib.layers.normalization.instancenormalization import InstanceNormalization
from tensorflow.python.ops.init_ops_v2 import _compute_fans

def load_model(filename):
    model = tf.keras.models.load_model(filename,
            custom_objects={
                    'MiniBatchStdDevLayer': MiniBatchStdDevLayer,
                    'InstanceNormalization':InstanceNormalization,
                    'Conv2DEQ' : Conv2DEQ,
                    'DenseEQ' : DenseEQ
                }
        )
    return model

# from https://stackoverflow.com/questions/60012406/how-may-i-do-equalized-learning-rate-with-tensorflow-2/62399708#62399708
class DenseEQ(layers.Dense):
    """
    Standard dense layer but includes learning rate equilization
    at runtime as per Karras et al. 2017.

    Inherits Dense layer and overides the call method.
    """
    def __init__(self, units, gain=np.sqrt(2), w_scale=True, **kwargs):
        if 'kernel_initializer' in kwargs and kwargs['kernel_initializer'] is not None:
            print('WARNING: cannot override kernel initializer for DenseEQ')
        kwargs.pop('kernel_initializer', None)
        super().__init__(units, kernel_initializer=tf.keras.initializers.RandomNormal(0, 1), **kwargs)
        # super().__init__(units, kernel_initializer=tf.keras.initializers.he_normal(), **kwargs)
        # super().__init__(units, kernel_initializer=tf.keras.initializers.ones, **kwargs)

        self.gain = gain
        self.w_scale = w_scale

    def build(self, input_shape):
        super().build(input_shape)

        # The number of inputs
        fan_in = np.prod(self.kernel.shape[:-1]) # [inputs, outputs]
        he_std = self.gain / np.sqrt(fan_in)  # He init
        # self.c = he_std / .87962566103423978 if self.w_scale == True else 1
        self.c = he_std if self.w_scale == True else 1

    def call(self, inputs):
        output = tf.matmul(inputs, self.kernel*self.c) # scale kernel
        if self.use_bias:
            output = tf.nn.bias_add(output, self.bias, data_format='N...C')
        if self.activation is not None:
            output = self.activation(output)
        return output

    def get_config(self):
        base_config = super(DenseEQ, self).get_config()
        base_config.pop('kernel_initializer')
        return dict(list(base_config.items()))

# from https://stackoverflow.com/questions/60012406/how-may-i-do-equalized-learning-rate-with-tensorflow-2/62399708#62399708
@keras_export('keras.layers.Conv2DEQ')
class Conv2DEQ(layers.Conv2D):
    """
    Standard Conv2D layer but includes learning rate equilization
    at runtime as per Karras et al. 2017.

    Inherits Conv2D layer and overrides the call method, following
    https://github.com/keras-team/keras/blob/master/keras/layers/convolutional.py

    """
    def __init__(self, filters, kernel_size, gain=np.sqrt(2), w_scale=True, **kwargs):
        if 'kernel_initializer' in kwargs and kwargs['kernel_initializer'] is not None:
            print('WARNING: cannot override kernel initializer for Conv2DEQ')
        kwargs.pop('kernel_initializer', None)
        super().__init__(filters, kernel_size, kernel_initializer=tf.keras.initializers.RandomNormal(0, 1), **kwargs)
        # super().__init__(filters, kernel_size, kernel_initializer=tf.keras.initializers.he_normal(), **kwargs)
        # super().__init__(filters, kernel_size, kernel_initializer=tf.keras.initializers.ones, **kwargs)

        self.gain = gain
        self.w_scale = w_scale

    def build(self, input_shape):
        super().build(input_shape)

        # The number of inputs
        fan_in = np.prod(self.kernel.shape[:-1]) # [inputs, outputs]
        he_std = self.gain / np.sqrt(fan_in)  # He init
        self.c = he_std if self.w_scale == True else 1


    def call(self, inputs):
        if self.rank == 2:
            # outputs = backend.conv2d(
            #     inputs,
            #     self.kernel*self.c, # scale kernel
            #     strides=self.strides,
            #     padding=self.padding,
            #     data_format=self.data_format,
            #     dilation_rate=self.dilation_rate)

            outputs = self._convolution_op(inputs, self.kernel*self.c)

        if self.use_bias:
            if self.data_format == 'channels_first':
                if self.rank == 1:
                    # nn.bias_add does not accept a 1D input tensor.
                    bias = array_ops.reshape(self.bias, (1, self.filters, 1))
                    outputs += bias
                else:
                    outputs = nn.bias_add(outputs, self.bias, data_format='NCHW')
            else:
                outputs = nn.bias_add(outputs, self.bias, data_format='NHWC')

        if self.activation is not None:
            return self.activation(outputs)
        return outputs

    def get_config(self):
        # TODO wscale and gain values
        base_config = super(Conv2DEQ, self).get_config()
        base_config.pop('kernel_initializer')
        return dict(list(base_config.items()))

class MiniBatchStdDevLayer(layers.Layer):
    def __init__(self, group_size=4, num_new_features=1, **kwargs):
        super(MiniBatchStdDevLayer, self).__init__(**kwargs)
        self.group_size=group_size
        self.num_new_features=num_new_features
    
    def get_config(self):
        config = {
            'group_size': self.group_size,
            'num_new_features': self.num_new_features
        }
        # base_config = super(layers.Layer, self).get_config()
        # return dict(list(base_config.items()) + list(config.items()))
        return dict(list(config.items()))
    
    def call(self, x):
        num_new_features = self.num_new_features
        group_size = tf.minimum(
            self.group_size, tf.shape(x)[0]
        )  # Minibatch must be divisible by (or smaller than) group_size.
        s = x.shape  # [NHWC]  Input shape.
        y = tf.reshape(
            x, [group_size, -1, num_new_features, s[1], s[2], s[3] // num_new_features]
        )  # [GMnHWc] Split minibatch into M groups of size G. Split channels into n channel groups c.
        y = tf.cast(y, tf.float32)  # [GMnHWc] Cast to FP32.
        y -= tf.reduce_mean(
            y, axis=0, keepdims=True
        )  # [GMnHWc] Subtract mean over group.
        y = tf.reduce_mean(tf.square(y), axis=0)  # [MnHWc]  Calc variance over group.
        y = tf.sqrt(y + 1e-8)  # [MnHWc]  Calc stddev over group.
        y = tf.reduce_mean(
            y, axis=[2, 3, 4], keepdims=True
        )  # [Mn111]  Take average over fmaps and pixels.
        y = tf.reduce_mean(y, axis=[4])  # [Mn11] Split channels into c channel groups
        y = tf.cast(y, x.dtype)  # [Mn11]  Cast back to original data type.
        y = tf.tile(
            y, [group_size, s[1], s[2], 1]
        )  # [NHWn]  Replicate over group and pixels.
        return tf.concat([x, y], axis=3)  # [NHWC]  Append as new fmap.