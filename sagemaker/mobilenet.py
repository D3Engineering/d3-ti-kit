import tensorflow as tf
from tensorflow import keras
from tensorflow.keras import layers
from tensorflow.keras.regularizers import l2
from tensorflow.keras.initializers import RandomUniform
import code

# math helpers

def makeDivisible(num, div):
  return int(num // div) * div

# copied from https://github.com/keras-team/keras-applications/blob/master/keras_applications/__init__.py
# and modified
def correctPad(img_size, kernel_size, stride):
    adjust = (1 - img_size % 2, 1 - img_size % 2)

    correct = (kernel_size // 2, kernel_size // 2)

    return ((correct[0] - adjust[0], correct[0]),
            (correct[1] - adjust[1], correct[1]))


def checkShape(actual, target, label=""):
  print("{}: {}".format(label, actual))
  if actual != target:
    raise ValueError("actual: {}, target: {}".format(actual, target))


# Layer shortcut functions
def addBN(x, name, axis=-1):
  # return x
  return layers.BatchNormalization(
    axis=axis,
    epsilon=1e-3,
    momentum=0.999,
    moving_variance_initializer=RandomUniform(minval=0.0, maxval=1.0),
    fused=True,
    name=name)(x)

  # return layers.BatchNormalization(axis=axis, fused=True, trainable=False, name=name)(x)

  # epsilon = 1e-3

  # batch_mean, batch_var = tf.nn.moments(x,[0])
  # scale = tf.Variable(tf.ones([32]))
  # beta = tf.Variable(tf.zeros([32]))
  # BN = tf.nn.batch_normalization(x, batch_mean, batch_var, beta, scale, epsilon, name=name)

  return BN


def addReLU6(x, name):
  return layers.ReLU(6., name=name)(x)


def addDropout(x, name, rate=0.5):
  return layers.Dropout(rate, name=name)(x)


def InvertedResidual(x,
  filters=None, stride=None,
  expand=1, alpha=1.0,
  block_id=None,config=None):

  isEval = config['model']['eval']
  useBias = isEval
  config = config['model']['regularize']['inv-layers']

  inputs = x
  input_channels = inputs.shape[-1]
  exp_channels = int(input_channels * expand)
  filters *= alpha


  prefix = 'block_{}_'.format(block_id)

  if block_id != 0:
    # expand
    expanded_conv_reg = None
    if config['expand']['reg']:
      expanded_conv_reg = l2(config['expand']['reg'])

    x = layers.Conv2D(
      exp_channels,
      (1,1),
      padding='same',
      use_bias=useBias,
      activation=None,
      name=prefix + 'expanded',
      activity_regularizer=expanded_conv_reg)(x)
    
    if not isEval:
      x = addBN(x, prefix + 'expand_BN')

    x = addReLU6(x, prefix + 'expand_relu')

    if (not isEval) and (not config['expand']['dropout'] is None):
      x = addDropout(x, prefix + 'expand_dropout', rate=config['expand']['dropout'])

  # depthwise
  strides = (stride,stride)

  if stride == 2:
    # code.interact(local=dict(globals(), **locals()))
    # exit(1)
    x = layers.ZeroPadding2D(
      padding=correctPad(x.shape[1], 3, stride),
      name=prefix + 'pad')(x)

  # single filter full depth convolution
  depthwise_conv_reg = None
  if config['depthwise']['reg']:
    depthwise_conv_reg = l2(config['depthwise']['reg'])

  x = layers.DepthwiseConv2D(
    (3,3),
    use_bias=useBias,
    activation=None,
    strides=strides,
    padding='same' if stride==1 else 'valid',
    name=prefix + 'depthwise',
    activity_regularizer=depthwise_conv_reg)(x)

  if not isEval:
    x = addBN(x, prefix + 'depth_BN')
  x = addReLU6(x, prefix + 'depthwise_relu')

  if (not isEval) and (not config['depthwise']['dropout'] is None):
    x = addDropout(x, prefix + 'depthwise_dropout', rate=config['depthwise']['dropout'])


  # project - multi-filter 1x1 convolution
  # TODO: why?
  filters = makeDivisible(filters, 8)

  project_conv_reg = None
  if config['project']['reg']:
    project_conv_reg = l2(config['project']['reg'])

  x = layers.Conv2D(
    filters,
    (1,1),
    padding='same',
    use_bias=useBias,
    activation=None,
    name=prefix + 'project',
    activity_regularizer=project_conv_reg)(x)

  if not isEval:
    x = addBN(x, prefix + 'project_BN')

  if (not isEval) and (not config['project']['dropout'] is None):
    x = addDropout(x, prefix + 'project_dropout', rate=config['project']['dropout'])

  # skip connection, if possible
  if stride == 1 and input_channels == filters:
    x = layers.Add(name=prefix + 'add')([inputs, x])

  return x
