import tensorflow as tf
import tensorflow.keras.backend as K
import numpy as np

from gan.model import latent_noise

# create loss function
cross_entropy = tf.keras.losses.BinaryCrossentropy(from_logits=True)

# requires sigmoid activation on output
def standard_discriminator_loss(G, D, opt, batch_size):
    real_loss = cross_entropy(tf.ones_like(real_output)*0.9, real_output)
    fake_loss = cross_entropy(tf.zeros_like(fake_output), fake_output)
    return real_loss + fake_loss

def standard_generator_loss(G, D, opt, batch_size):
    return cross_entropy(tf.ones_like(fake_output)*0.9, fake_output)

# requires linear activation on output
def wgan_discriminator_loss(
            G, 
            D,
            opt,
            noise,
            reals,                  # real images batch
            sketch,
            wgan_lambda=10.0,       # Weight for the gradient penalty term.
            wgan_epsilon=0.001,     # Weight for the epsilon term, \epsilon_{drift}.
            wgan_target=1.0,        # Target value for gradient magnitudes.
        ):

    fake_images_out = G([sketch, noise], training=True)
    fake_scores_out = D([fake_images_out, sketch], training=True)
    real_scores_out = D([reals, sketch], training=True)
    
    loss = fake_scores_out - real_scores_out

    # gradient penalty
    mixing_factors = tf.random.uniform(
        [np.shape(noise)[0], 1, 1, 1], 0.0, 1.0
    )
    # mixing_factors = [mixing_factors for i in range(len(fake_images_out))]
    # mixed_images_out = [mix * real + (1.0 - mix) * fake for real, fake, mix in zip(reals, fake_images_out, mixing_factors)]
    mixed_images_out = mixing_factors * reals + (1.0 - mixing_factors) * fake_images_out
    
    mixed_scores_out = tf.reduce_mean(D([mixed_images_out, sketch], training=True))

    # gradient of mixed_scores_out with respect to mixed_images_out
    grad = K.gradients(mixed_scores_out, mixed_images_out)[0]
    mixed_norms = tf.sqrt(tf.reduce_sum(tf.square(grad)))

    gradient_penalty = tf.square(mixed_norms - wgan_target)
    
    loss += gradient_penalty * (wgan_lambda / (wgan_target ** 2))

    epsilon_penalty = tf.square(real_scores_out) * wgan_epsilon
    loss += epsilon_penalty
    return loss

def wgan_generator_wloss_mae(
            G, 
            D,
            opt,
            noise,
            reals,                  # real images batch
            sketch,
            l1_weight=100.0,
            gan_weight=1.0):

    fake_images_out = G([sketch, noise], training=True)
    fake_scores_out = D([fake_images_out, sketch], training=True)

    gen_loss_L1 = tf.reduce_mean(tf.abs(reals - fake_images_out))
    gen_loss_GAN = - tf.reduce_mean(fake_scores_out)
    gen_loss = gen_loss_GAN * gan_weight + gen_loss_L1 * l1_weight

    return gen_loss


# RelativisticAverageHingeGAN loss
def relhinge_discriminator_loss(
            G,
            D,
            opt,
            batch_size,
            reals,                  # real images batch
            sketch,
            hinge_lambda=10.0,       # Weight for the gradient penalty term.
            hinge_epsilon=0.001,     # Weight for the epsilon term, \epsilon_{drift}.
            hinge_target=1.0,        # Target value for gradient magnitudes.
    ):

    latents = latent_noise(batch_size, 1)

    fake_images_out = G([sketch, latents], training=True)

    fake_scores_out = D([fake_images_out, sketch], training=True)
    real_scores_out = D([reals, sketch], training=True)

    # difference between real and fake:
    real_logit = real_scores_out - tf.reduce_mean(fake_scores_out)

    # difference between fake and real samples
    fake_logit = fake_scores_out - tf.reduce_mean(real_scores_out)

    loss = (tf.reduce_mean(tf.nn.relu(1.0 - real_logit))
            + tf.reduce_mean(tf.nn.relu(1.0 + fake_logit)))

    # # gradient penalty
    # mixing_factors = tf.random.uniform(
    #     [batch_size, 1, 1, 1], 0.0, 1.0
    # )
    # mixing_factors = [mixing_factors for i in range(len(fake_images_out))]
    # # mix * real + (1.0 - mix) * fake
    # mixed_images_out = [mix * real + (1.0 - mix) * fake for real, fake, mix in zip(reals, fake_images_out, mixing_factors)]
    
    # mixed_scores_out = tf.reduce_mean(D(mixed_images_out, training=True))

    # # gradient of mixed_scores_out with respect to mixed_images_out
    # grad = K.gradients(mixed_scores_out, mixed_images_out)[0]
    # mixed_norms = tf.sqrt(tf.reduce_sum(tf.square(grad)))

    # gradient_penalty = tf.square(mixed_norms - hinge_target)
    
    # loss += gradient_penalty * (hinge_lambda / (hinge_target ** 2))

    # return the loss
    return loss

def relhinge_generator_loss(G, D, opt, batch_size, reals, sketch):

    latents = latent_noise(batch_size, 1)

    fake_images_out = G([sketch, latents], training=True)

    fake_scores_out = tf.reduce_mean(D([fake_images_out, sketch], training=True))
    real_scores_out = tf.reduce_mean(D([reals, sketch], training=True))


    # difference between real and fake:
    real_logit = real_scores_out - tf.reduce_mean(fake_scores_out)

    # difference between fake and real samples
    fake_logit = fake_scores_out - tf.reduce_mean(real_scores_out)

    # return the loss
    return (tf.reduce_mean(tf.nn.relu(1.0 + real_logit))
            + tf.reduce_mean(tf.nn.relu(1.0 - fake_logit)))


# hinge, use wgan_generator_wloss

def hinge_discriminator_loss(
            G,
            D,
            opt,
            batch_size,
            reals,                  # real images batch
            sketch,
            hinge_lambda=10.0,       # Weight for the gradient penalty term.
            hinge_epsilon=0.001,     # Weight for the epsilon term, \epsilon_{drift}.
            hinge_target=1.0,        # Target value for gradient magnitudes.
    ):

    latents = latent_noise(batch_size, G.input_shape[1])

    fake_images_out = G(latents, training=True)

    fake_scores_out = D(fake_images_out, training=True)
    real_scores_out = D(reals, training=True)

    loss = tf.maximum(0.0, tf.reduce_mean(1.0 + fake_scores_out)) + tf.maximum(0.0, tf.reduce_mean(1.0 - real_scores_out))

    # gradient penalty
    mixing_factors = tf.random.uniform(
        [batch_size, 1, 1, 1], 0.0, 1.0
    )
    mixing_factors = [mixing_factors for i in range(len(fake_images_out))]
    # mix * real + (1.0 - mix) * fake
    mixed_images_out = [mix * real + (1.0 - mix) * fake for real, fake, mix in zip(reals, fake_images_out, mixing_factors)]
    
    mixed_scores_out = tf.reduce_mean(D(mixed_images_out, training=True))

    # gradient of mixed_scores_out with respect to mixed_images_out
    grad = K.gradients(mixed_scores_out, mixed_images_out)[0]
    mixed_norms = tf.sqrt(tf.reduce_sum(tf.square(grad)))

    gradient_penalty = tf.square(mixed_norms - hinge_target)
    
    loss += gradient_penalty * (hinge_lambda / (hinge_target ** 2))

    # return the loss
    return loss


# others 

def G_logistic_nonstaturating(
                G,
                D,
                opt,
                batch_size,
                reals,                  # real images batch
                sketch
    ):


    fake_images_out = G([sketch, noise], training=True)
    fake_scores_out = D([fake_images_out, sketch], training=True)
    loss = tf.nn.softplus(-fake_scores_out)  # -log(logistic(fake_scores_out))
    return loss


def D_logistic_simplegp(
                G,
                D,
                opt,
                noise,
                reals,                  # real images batch
                sketch,
                r1_gamma=10.0,
                r2_gamma=0.0
    ):


    fake_images_out = G([sketch, noise], training=True)
    fake_scores_out = D([fake_images_out, sketch], training=True)
    real_scores_out = D([reals, sketch], training=True)

    loss = tf.nn.softplus(fake_scores_out)  # -log(1 - exp(fake_scores_out))
    loss += tf.nn.softplus(
        -real_scores_out
    )

    

    if r1_gamma != 0.0:
        real_loss = tf.reduce_sum(real_scores_out)
        real_grads = tf.gradients(real_loss, reals)
        r1_penalty = tf.reduce_mean(
            [
                tf.reduce_sum(tf.square(real_grad), axis=[1, 2, 3])
                for real_grad in real_grads
            ]
        )

        loss += r1_penalty * (r1_gamma * 0.5)

    if r2_gamma != 0.0:
        fake_loss = tf.reduce_sum(fake_scores_out)
        fake_grads = tf.gradients(fake_loss, [fake_images_out])
        r2_penalty = tf.reduce_mean(
            [
                tf.reduce_sum(tf.square(fake_grad), axis=[1, 2, 3])
                for fake_grad in fake_grads
            ]
        )

        loss += r2_penalty * (r2_gamma * 0.5)

    loss = tf.where(tf.math.is_nan(loss), tf.zeros_like(loss), loss)
    return loss


EPS = 1e-12

def D_pix2pix_original( # use sigmoid activation on final layer
                G,
                D,
                opt,
                noise,
                reals,                  # real images batch
                sketch
    ):


    fake_images_out = G([sketch, noise], training=True)
    fake_scores_out = D([fake_images_out, sketch], training=True)
    real_scores_out = D([reals, sketch], training=True)

    discrim_loss = tf.reduce_mean(-(tf.math.log(real_scores_out + EPS) + tf.math.log((1 - fake_scores_out) + EPS)))


    # print(f'fake_scores_out: {tf.reduce_mean(fake_scores_out)}, real_scores_out: {tf.reduce_mean(real_scores_out)}, discrim_loss: {discrim_loss}')

    return discrim_loss

def D_pix2pix(
                G,
                D,
                opt,
                noise,
                reals,                  # real images batch
                sketch
    ):


    fake_images_out = G([sketch, noise], training=True)
    fake_scores_out = D([fake_images_out, sketch], training=True)
    real_scores_out = D([reals, sketch], training=True)

    # fake_scores_out = tf.clip_by_value(fake_scores_out, 0.0, 1.0)
    # real_scores_out = tf.clip_by_value(real_scores_out, 0.0, 1.0)

    # discrim_loss = tf.reduce_mean(-(tf.math.log(real_scores_out + EPS) + tf.math.log((1 - fake_scores_out) + EPS)))

    discrim_loss = tf.nn.softplus(fake_scores_out)  # -log(1 - exp(fake_scores_out))
    discrim_loss += tf.nn.softplus(
        -real_scores_out
    )

    # print(f'fake_scores_out: {tf.reduce_mean(fake_scores_out)}, real_scores_out: {tf.reduce_mean(real_scores_out)}, discrim_loss: {discrim_loss}')

    return discrim_loss


def G_mae_pix2pix(
                G,
                D,
                opt,
                noise,
                reals,                  # real images batch
                sketch,
                l1_weight=100.0,
                gan_weight=1.0
    ):


    fake_images_out = G([sketch, noise], training=True)
    fake_scores_out = D([fake_images_out, sketch], training=True)

    # print(f'gen_loss_GAN: {gen_loss_GAN}, gen_loss_L1: {gen_loss_L1}')

    gen_loss_L1 = tf.reduce_mean(tf.abs(reals - fake_images_out))
    gen_loss_GAN = tf.reduce_mean(-tf.math.log(fake_scores_out + EPS))
    gen_loss = gen_loss_GAN * gan_weight + gen_loss_L1 * l1_weight
    return gen_loss

def G_mae_msg(
                G,
                D,
                opt,
                noise,
                reals,                  # real images batch
                sketch,
                l1_weight=10.0,
                gan_weight=.10
    ):


    fake_images_out = G([sketch, noise], training=True)
    fake_scores_out = D([fake_images_out, sketch], training=True)


    # fake_scores_out = tf.clip_by_value(fake_scores_out, 0.0, 1.0)
    # gen_loss_GAN = tf.reduce_mean(-tf.math.log(fake_scores_out + EPS))
    
    gen_loss_GAN = tf.nn.softplus(-fake_scores_out)

    gen_loss_L1 = 0
    if isinstance(fake_images_out, list):
        for r, f in zip(reals, fake_images_out):
            gen_loss_L1 += tf.reduce_mean(tf.abs(r - f))
    else:
        gen_loss_L1 = tf.reduce_mean(tf.abs(reals - fake_images_out))

    gen_loss = gen_loss_L1 * l1_weight + gen_loss_GAN * gan_weight

    # print(f'gen_loss_GAN: {gen_loss_GAN}, gen_loss_L1: {gen_loss_L1}')

    # gen_loss_GAN = tf.reduce_mean(-tf.math.log(fake_scores_out + EPS))
    return gen_loss