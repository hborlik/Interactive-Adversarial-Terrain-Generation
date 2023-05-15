import tensorflow as tf
from tensorflow.keras.layers import *
from tensorflow.keras.models import Model
from tensorflow.keras.initializers import RandomNormal
import tensorflow.keras.backend as K
from keras_contrib.layers.normalization.instancenormalization import InstanceNormalization
import numpy as np

import gan.custom_layers as custom_layers

def add_noise(img, std, batch_size):
    noise = layers.GaussianNoise(std)(img)
    # noise = tf.random.normal(shape=[batch_size, img.shape[1], img.shape[2], img.shape[3]], mean=0.0, stddev=std, dtype=tf.float32)
    return noise

@tf.function
def latent_noise(batch_size, dimension, norm=True):
    noise = tf.random.normal([batch_size, dimension, dimension, 128])
    if norm:
        noise = (noise
                           / tf.norm(noise, axis=(1, 2), keepdims=True)
                           * (dimension))
    return noise

def make_filter(f):
    f = np.array(f, dtype=np.float32)
    if f.ndim == 1:
        f = f[:, np.newaxis] * f[np.newaxis, :]
    f /= np.sum(f)

    f = f[:, :, np.newaxis, np.newaxis]
    # f = np.tile(f, [1, 1, int(x.shape[3]), 1])
    # f = tf.cast(f, x.dtype)
    return f

def blur2D(x, f):
    f = tf.tile(f, [1, 1, int(x.shape[3]), 1])
    f = tf.cast(f, x.dtype)
    return tf.nn.depthwise_conv2d(x, f, [1, 1, 1, 1], 'SAME')

def UNet(shape, use_msg=False):
    init=tf.keras.initializers.GlorotUniform()
    # init = RandomNormal(stddev=0.02)
    blur_filter=[
        1,
        2,
        1
    ]
    blur_filter = make_filter(blur_filter) if blur_filter else None

    def act(x):
        return LeakyReLU(alpha=0.2)(x)
        # return ReLU()(x)

    def blur(x):
        return x
        # return blur2D(x, blur_filter) if blur_filter is not None else x

    def block_epilogue(x):
        x = InstanceNormalization()(blur(x))
        x = act(x)
        return x

    def decode(x, fmaps, s=4, dropout=0.0, bn=True):
        up = UpSampling2D(size=(2, 2))(x)
        up = SeparableConv2D(fmaps, s, strides=(1, 1), padding='same', depthwise_initializer=init, pointwise_initializer=init)(up)
        up = block_epilogue(up)
        if dropout != 0.0:
            up = Dropout(rate=1 - dropout)(up)
        if bn:
            up = BatchNormalization()(up)
        return up

    def genconv(x, fmaps):
        # x = SeparableConv2D(fmaps, 4, strides=(2, 2), padding='same', depthwise_initializer=init, pointwise_initializer=init)(x)
        # x = Conv2D(fmaps, 4, strides=(2, 2), padding='same', kernel_initializer=init)(x)
        x = custom_layers.Conv2DEQ(fmaps, 4, strides=(2, 2), padding='same')(x)
        return x

    def to_height(x):
        # return Conv2D(1, 1, strides=(1, 1), activation='tanh', padding='same', kernel_initializer=init)(x)
        return SeparableConv2D(1, 1, strides=(1, 1), activation='tanh', padding='same', depthwise_initializer=init, pointwise_initializer=init)(x)

    inputs = Input(shape)
    output = []

    layer_specs = [
        64,
        126,
        256,
        512,
        512,
        512,
        512,
        512
    ]

    layers = [genconv(inputs, 32)]
    for spec in layer_specs:
        out = LeakyReLU(alpha=0.2)(layers[-1])
        out = genconv(out, spec)
        out = BatchNormalization()(out)
        layers.append(out)

    print(out.shape)
    noise = Input((K.int_shape(out)[1], K.int_shape(out)[2], 128))
    layers[-1] = Concatenate()([layers[-1], noise])

    layer_specs = [
        (512, 0.5),
        (512, 0.5),
        (512, 0.5),
        (512, 0.0),
        (256, 0.0),
        (128, 0.0),
        (64, 0.0),
        (32, 0.0)
    ]
    
    num_encoder_layers = len(layers)
    for decoder_layer, (out_channels, dropout) in enumerate(layer_specs):
        skip_layer = num_encoder_layers - decoder_layer - 1
        if decoder_layer == 0:
            # first decoder layer doesn't have skip connections
            # since it is directly connected to the skip_layer
            input_layer = layers[-1]
        else:
            input_layer = Concatenate()([layers[-1], layers[skip_layer]])

        up = LeakyReLU(alpha=0.2)(input_layer)
        up = UpSampling2D(size=(2, 2))(up)
        # up = Conv2D(out_channels, 4, strides=(1, 1), padding='same', kernel_initializer=init)(up)
        up = custom_layers.Conv2DEQ(out_channels, 4, strides=(1, 1), padding='same')(up)
        # up = SeparableConv2D(out_channels, 4, strides=(1, 1), padding='same', depthwise_initializer=init, pointwise_initializer=init)(up)
        up = BatchNormalization()(up)

        if use_msg:
            output.append(to_height(up))
        
        if dropout > 0.0:
            up = Dropout(rate=1 - dropout)(up)

        layers.append(up)

    final_conv = Concatenate()([layers[-1], layers[0]])
    final_conv = LeakyReLU(alpha=0.2)(final_conv)
    final_conv = UpSampling2D(size=(2, 2))(final_conv)
    # final_conv = to_height(final_conv)
    # final_conv = SeparableConv2D(1, 4, strides=(1, 1), padding='same', activation='tanh', depthwise_initializer=init, pointwise_initializer=init)(final_conv)
    # final_conv = Conv2D(1, 4, strides=(1, 1), padding='same', activation='linear', kernel_initializer=init)(final_conv)
    final_conv = custom_layers.Conv2DEQ(1, 4, strides=(1, 1), padding='same', activation='linear')(final_conv)

    if use_msg:
        output.append(final_conv)
        output.reverse()
        print([x.shape for x in output])
    else:
        output = final_conv

    model = Model([inputs, noise], output)
    model.summary()
    return model


def patch_discriminator(shape, sketch_shape, use_msg=False):
    init=tf.keras.initializers.GlorotUniform()
    # init = RandomNormal(stddev=0.02)
    final_activation = 'linear'
    blur_filter=[
        1,
        2,
        1
    ]
    blur_filter = make_filter(blur_filter) if blur_filter else None

    def blur(x):
        # return x
        return blur2D(x, blur_filter) if blur_filter is not None else x

    def act(x):
        return LeakyReLU(alpha=0.2)(x)

    def block_epilogue(x, bn=False, rect=True):
        # x = InstanceNormalization()(blur(x))
        if bn:
            x = BatchNormalization()(x)
        if rect:
            x = act(x)
        return x

    def encode(x, fmaps, strides=1, bn=True, rect=True):
        # conv = AvgPool2D(pool_size=(2, 2))(conv)
        # conv = Conv2D(fmaps, 4, strides=(strides, strides), padding='same', kernel_initializer=init)(x)
        conv = custom_layers.Conv2DEQ(fmaps, 4, strides=(strides, strides), padding='same')(x)
        # conv = SeparableConv2D(fmaps, 4, strides=(strides, strides), padding='same', depthwise_initializer=init, pointwise_initializer=init)(x)
        conv = block_epilogue(conv, bn=bn, rect=rect)
        return conv

    def from_height(x, fmaps):
        # conv = Conv2D(fmaps, 1, padding='same', kernel_initializer=init)(x)
        conv = SeparableConv2D(fmaps, 1, padding='same', depthwise_initializer=init, pointwise_initializer=init)(x)
        conv = act(conv)
        return conv

    inputs = []
    def conditional_msg_input(x):
        if use_msg:
            inputn = Input(shape=(x.shape[1], x.shape[2], 1))
            inputs.append(inputn)
            inputn = from_height(inputn, x.shape[3])
            x = Concatenate()([inputn, x])
        return x

    in_image = Input(shape=shape)
    inputs.append(in_image)
    cond_image = Input(shape=sketch_shape)

    # in_image_2 = GaussianNoise(0.2)(in_image)
    # in_image_2 = custom_layers.MiniBatchStdDevLayer()(in_image)
    conc_img = Concatenate()([in_image, cond_image])

    d = encode(conc_img, 64, strides=2, bn=False)
    d = conditional_msg_input(d)
    d = encode(d, 128, strides=2)
    d = conditional_msg_input(d)
    d = encode(d, 256, strides=2)
    d = conditional_msg_input(d)
    d = encode(d, 512, strides=2)
    # d = conditional_msg_input(d)
    # d = encode(d, 512, strides=2)
    # d = conditional_msg_input(d)
    # d = encode(d, 512, strides=2)
    # d = conditional_msg_input(d)
    # d = encode(d, 512, strides=2)
    # d = conditional_msg_input(d)
    output = encode(d, 1, strides=1, bn=False, rect=False)

    # d = Dropout(0.5)(d)
    output = Flatten()(output)
    output = custom_layers.DenseEQ(1, activation=final_activation)(output)
    # output = tf.sigmoid(output)

    if use_msg:
        in_image = inputs
        print([x.shape for x in in_image])
    
    model = Model([in_image, cond_image], output)
    model.summary()

    return model


def mount_discriminator_generator(g, d, image_shape):
    d.trainable = False
    input_gen = Input(shape=image_shape)
    input_noise = Input(shape=(32, 32, 1))
    gen_out = g([input_gen, input_noise])
    output_d = d([gen_out, input_gen])
    model = Model(inputs=[input_gen, input_noise], outputs=[output_d, gen_out])
    model.summary()

    return model