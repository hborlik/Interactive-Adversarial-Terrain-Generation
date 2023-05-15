import tensorflow as tf
import numpy as np
import random
import time
from pathlib import Path
import matplotlib.pyplot as plt
from tensorflow.keras import layers, models
from tensorflow.python.ops import state_ops
from tensorflow.keras.optimizers import Adam, RMSprop

import gan.model as GAN
import gan.custom_layers as custom_layers
from gan.custom_layers import MiniBatchStdDevLayer
from gan.custom_layers import Conv2DEQ, DenseEQ, load_model
import loss

# # loss functions

# discriminator_loss = loss.D_logistic_simplegp
# generator_loss = loss.G_mae_msg

discriminator_loss = loss.wgan_discriminator_loss
generator_loss = loss.wgan_generator_wloss_mae


ema_decay = 0.999

adam_b1 = 0.5
adam_b2 = 0.999

USE_MULTI_SCALE_GRAD = False

save_epoch_interval = 2

# optimizers 
generator_optimizer = tf.keras.optimizers.Adam(0.003, beta_1=adam_b1, beta_2=adam_b2)
discriminator_optimizer = tf.keras.optimizers.Adam(0.003, beta_1=adam_b1, beta_2=adam_b2)


files = list(Path('./data/input').glob('*.npz'))
np.random.shuffle(files)
def data_generator():
    i = 0
    for f in files:
        with np.load(f) as data:
            i = i + 1
            for x, y in zip(data['x'], data['y']):
                # if bool(random.getrandbits(1)):
                #     x[..., 2] = np.zeros_like(x[..., 2])
                # elif bool(random.getrandbits(1)):
                #     x[..., 1] = np.zeros_like(x[..., 1])
                # elif bool(random.getrandbits(1)):
                #     x[..., 0] = np.zeros_like(x[..., 0])
                yield x, y


def decode_img(imgx, imgy):
    # imgx, imgy = img
    # Use `convert_image_dtype` to convert to floats in the [0,1] range.
    # img = tf.image.convert_image_dtype(img, tf.float32)
    imgx = tf.cast(imgx, tf.float32)

    a = tf.concat([imgx, imgy], axis=-1)
    a = tf.image.random_flip_up_down(a)
    a = tf.image.random_flip_left_right(a)
    a = tf.image.random_crop(a, (512, 512, a.shape[-1]))
    # a = tf.image.resize(a, (512,512))

    imgx = a[..., 0:3]
    imgy = a[..., -1:]

    rv = tf.cast(tf.random.uniform(shape=(1, 1, 3), minval=0, maxval=2, dtype=tf.int32) , dtype=tf.float32)
    rv = tf.tile(rv, [512, 512, 1])
    imgx = tf.multiply(imgx, rv)

    imgx = (imgx - 127.0) / 127.0
    imgx = tf.clip_by_value(imgx, -1.0, 1.0)

    # print (a.shape)


    # imgy = tf.cast(imgy, tf.float32)
    # imgy = (imgy - 127.0) / 127.0
    imgy = tf.image.per_image_standardization(imgy)
    return imgx, imgy


def make_dataset(batch_size=32):
    dataset = tf.data.Dataset.from_generator(data_generator,
        output_signature= (tf.TensorSpec((1200, 1200, 3), dtype=tf.dtypes.uint8), tf.TensorSpec((1200, 1200, 1), tf.dtypes.float32)))
    dataset = dataset.repeat()
    dataset = dataset.map(lambda imgx, imgy : decode_img(imgx, imgy), num_parallel_calls=4)
    return dataset.shuffle(batch_size*4).batch(batch_size).prefetch(2)

def generate_real_samples(X, Y_target, n_samples, patch_size):
    gt = Y_target
    y = np.ones((n_samples, patch_size, patch_size, 1))
    return X, y, gt


def generate_fake_samples(g_model, X, patch_size):
    w_noise = np.random.normal(0, 1, (X.shape[0], 32, 32, 1))
    X = g_model.predict([X, w_noise])
    y = np.zeros((len(X), patch_size, patch_size, 1))
    return X, y

def sample_images(generator, sh_model, source, target, idx, dir='./generated/'):
    # print(target.shape)
    target = target * 0.5 + 0.5
    mint = np.min(target)
    target = (target - mint) / (np.max(target) - mint)
    target = np.uint8(target * 255)
    w_noise = GAN.latent_noise(1, generator.input_shape[1][1])
    predicted = generator.predict([source, w_noise])
    sh_model_predicted = sh_model.predict([source, w_noise])
    assert(np.all(np.array([x.shape == y.shape for x, y in zip(predicted, sh_model_predicted)])))
    if USE_MULTI_SCALE_GRAD:
        fig = plt.figure(figsize=(2, len(predicted)), dpi=500)
        for i in range(len(predicted)):
            plt.subplot(len(predicted), 2, 2*i+1)
            plt.imshow((predicted[i][0]) * 0.5 + 0.5, cmap='gray')
            plt.axis('off')

            plt.subplot(len(predicted), 2, 2*i+2)
            plt.imshow((sh_model_predicted[i][0]) * 0.5 + 0.5, cmap='gray')
            plt.axis('off')
        plt.savefig(f'generated/sketch_output{idx}_multires.png')

        predicted = predicted[0]
        sh_model_predicted = sh_model_predicted[0]
    print("min predicted", np.min(predicted[0, ...]))
    print("max predicted", np.max(predicted[0, ...]))
    print("min sh_model_predicted", np.min(sh_model_predicted[0, ...]))
    print("max sh_model_predicted", np.max(sh_model_predicted[0, ...]))
    predicted = predicted * 0.5 + 0.5
    sh_model_predicted = sh_model_predicted * 0.5 + 0.5
    mint = np.min(sh_model_predicted)
    sh_model_predicted = (sh_model_predicted - mint) / (np.max(sh_model_predicted) - mint)
    mint = np.min(predicted)
    predicted = (predicted - mint) / (np.max(predicted) - mint)
    im = np.uint8(predicted[0, ...] * 255)
    im_sh = np.uint8(sh_model_predicted[0, ...] * 255)
    im_source = source[0, ...] * 127.5 + 127.5#np.uint8(source[0, ...] * 127.5 + 127.5)
    # print(np.min(im_source[..., 2]))
    # print(np.max(im_source[..., 2]))
    # print(im_source.shape)
    im_c = np.concatenate((np.squeeze(im, axis=-1), np.squeeze(im_sh, axis=-1), np.squeeze(target, axis=-1),
                           im_source[..., 0], im_source[..., 1], im_source[..., 2]), axis=1)
    plt.imsave('./generated/sketch_output' + str(idx) + '.png', im_c, cmap='terrain')

def test_gan(id):
    terrain_generator = load_model(f'terrain_generator{id}.h5')
    i = 0
    for data in make_dataset():
        XTrain, YTrain = data
        print(XTrain.shape)
        print(YTrain.shape)
        for x in range(XTrain.shape[0]):
            source = XTrain[x:x + 1, ...]
            target = YTrain[x, ...]
            target = target * 0.5 + 0.5
            mint = np.min(target)
            target = (target - mint) / (np.max(target) - mint)
            target = np.uint8(target * 255)
            w_noise = GAN.latent_noise(1, terrain_generator.input_shape[1][1])
            predicted = terrain_generator.predict([source, w_noise])
            predicted = predicted * 0.5 + 0.5
            mint = np.min(predicted)
            predicted = (predicted - mint) / (np.max(predicted) - mint)
            im = np.uint8(predicted[0, ...] * 255)
            im_source = source[0, ...] * 127.5 + 127.5
            im_c = np.concatenate((np.squeeze(im, axis=-1), np.squeeze(target, axis=-1),
                           im_source[..., 0], im_source[..., 1], im_source[..., 2]), axis=1)
            plt.imsave('./generated/test_sketch_output' + str(x) + '.png', im_c, cmap='terrain')
            # sketch = np.stack(im_source[..., 0], im_source[..., 1], im_source[..., 2])
            # im_c = np.concatenate((np.squeeze(im, axis=-1), np.squeeze(target, axis=-1),
            #                im_source[..., 0], im_source[..., 1], im_source[..., 2]), axis=1)
            # plt.imsave('./generated/test_sketch_output' + str(x) + '.png', im_c, cmap='terrain')
            
        if i > 3:
            break
        i += 1

def replacenan(t):
    return tf.where(tf.math.is_nan(t), tf.zeros_like(t), t)

def plot_loss(gloss, dloss, show=False):
    plt.figure()
    plt.plot(gloss)
    plt.plot(dloss)
    plt.legend(["G loss", "D loss"])
    plt.savefig('generated/loss.png')
    if show:
        plt.show()
    else:
        plt.close()

def plot_grad(g):
    plt.figure()
    plt.plot(g)
    plt.legend(["grad"])
    plt.title("gradient")
    plt.savefig('generated/grad_top.png')
    plt.close()

def plot_mse(mse):
    if len(mse) != 0:
        mse = np.array(mse)
        plt.figure()
        for i in range(mse.shape[1]):
            plt.plot(np.array(mse)[:, i])
        plt.savefig("generated/mse.png")
        plt.close()

@tf.function
def train_step(image_batch, sketch_batch, generator, discriminator, sh_model):
    batch_size = image_batch.shape[0]
    noise_shape = generator.input_shape[1][1]
    with tf.GradientTape() as gen_tape, tf.GradientTape() as disc_tape:
        # generated_images = generator(sketch_batch, training=True)
        if USE_MULTI_SCALE_GRAD:
            image_batch = [image_batch] + [layers.AveragePooling2D(pool_size=int(2**i))(image_batch) for i in range(1, 8)]

        noise = GAN.latent_noise(batch_size, noise_shape, False)

        disc_loss = discriminator_loss(generator, discriminator, discriminator_optimizer, noise, image_batch, sketch_batch)
        gradients_of_discriminator = disc_tape.gradient(disc_loss, discriminator.trainable_variables)
        discriminator_optimizer.apply_gradients(zip(gradients_of_discriminator, discriminator.trainable_variables))


        gen_loss = generator_loss(generator, discriminator, generator_optimizer, noise, image_batch, sketch_batch)
        gradients_of_generator = gen_tape.gradient(gen_loss, generator.trainable_variables)
        generator_optimizer.apply_gradients(zip(gradients_of_generator, generator.trainable_variables))

    update_ema(sh_model, generator, ema_decay)

    return gen_loss, disc_loss, gradients_of_generator

def update_ema(shadow, src, beta):
    assert len(shadow.trainable_variables) == len(src.trainable_variables)
    with tf.name_scope("EMA"):
        # state_ops.assign_sub(v, (v - value) * beta, name=scope)
        for shadow_var, var in zip(shadow.trainable_variables, src.trainable_variables):
            shadow_var.assign(beta * shadow_var + (1 - beta) * var)

def train_gan(dataset, load_from = None):
    input_shape_gen = (512, 512, 3)
    input_shape_disc = (512, 512, 1)

    if load_from is None:
        terrain_generator = GAN.UNet(input_shape_gen, use_msg=USE_MULTI_SCALE_GRAD)
        terrain_discriminator = GAN.patch_discriminator(input_shape_disc, input_shape_gen, use_msg=USE_MULTI_SCALE_GRAD)
        sh_model = tf.keras.models.clone_model(terrain_generator)
    else:
        terrain_generator = custom_layers.load_model(f'terrain_generator{load_from}.h5')
        terrain_discriminator = custom_layers.load_model(f'terrain_discriminator{load_from}.h5')
        sh_model = custom_layers.load_model(f'terrain_sh_model{load_from}.h5')

    update_ema(sh_model, terrain_generator, 0)

    n_epochs = 500
    n_steps = 10000
    patch_size = 32
    feedback_factor = 100
    min_loss = 999
    avg_loss = 0
    avg_dloss = 0
    i = load_from if load_from is not None else 0
    gloss = []
    dloss = []
    grad_top = []
    for epoch in range(i, n_epochs):
        genl = []
        discl = []
        gen_grad = []
        for data, n in zip(dataset, range(n_steps)):
            XTrain, YTrain = data
            Y_target = replacenan(YTrain)
            X_real = replacenan(XTrain)

            # training
            _genl, _discl, _gen_grad = train_step(Y_target, X_real, terrain_generator, terrain_discriminator, sh_model)
            assert not np.any(np.isnan(_genl.numpy()))
            assert not np.any(np.isnan(_discl.numpy()))

            # print(tf.norm(sh_model.trainable_variables[0]).numpy(), tf.norm(terrain_generator.trainable_variables[0]).numpy())
            
            genl.append(_genl)
            discl.append(_discl)
            gen_grad.append(tf.norm(_gen_grad[-1]))

            if n % feedback_factor == 0:
                gla = tf.reduce_mean(genl)
                gloss.append(gla)
                dla = tf.reduce_mean(discl)
                dloss.append(dla)
                gt = tf.reduce_mean(gen_grad)
                grad_top.append(gt)

                print(f'{n} losses G: {gla}, D: {dla}')
                
                genl = []
                discl = []
                gen_grad = []
            
            # d_loss = (loss_discriminator_fake + loss_discriminator_real) / 2

            # avg_dloss = avg_dloss + (d_loss - avg_dloss) / (i + 1)
            # avg_loss = avg_loss + (losses_composite[0] - avg_loss) / (i + 1)
            # print('total loss:' + str(avg_loss) + ' d_loss:' + str(avg_dloss) + f' {i}')
            if n % (n_steps//10) == 0:
                sample_images(terrain_generator, sh_model, X_real[0:1, ...], Y_target[0, ...], f'{epoch}_{n}')
                print(n)
            if n % (n_steps//10) == 0:
                plot_loss(gloss, dloss)
                # plot_mse(mse)
                plot_grad(grad_top)
        print(f'EPOCH: {epoch}')
        if epoch % save_epoch_interval == 0:
            terrain_discriminator.save('terrain_discriminator' + str(epoch) + '.h5', True)
            terrain_generator.save('terrain_generator' + str(epoch) + '.h5', True)
            sh_model.save('terrain_sh_model' + str(epoch) + '.h5', True)


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

test_gan(48)
# train_gan(make_dataset(4), 32)
