import tensorflow as tf
from tensorflow import keras
from tensorflow.keras import layers, backend, metrics, activations
from tensorflow.keras.optimizers import SGD, Adam
from tensorflow.keras.regularizers import l2
import tensorflow.keras.backend as K
from tensorflow.compat.v1.saved_model import builder, SERVING
from tensorflow.compat.v1.saved_model.signature_def_utils import predict_signature_def
import tensorflow.compat.v1 as tfv1
import numpy as np



from tensorflow.keras.callbacks import \
    ReduceLROnPlateau, \
    LearningRateScheduler, \
    Callback, \
    ModelCheckpoint, \
    EarlyStopping \

from tensorflow.keras.regularizers import l2

import matplotlib.pyplot as plt

import code
import os
import pprint

import mobilenet as mnet


# tf.compat.v1.disable_eager_execution()

def CreateModel(config):
  config_reg = config['model']['regularize']
  config_model = config['model']
  isEval = config_model['eval']
  useBias = isEval

  # initial convolution

  # TODO: figure out why I need to do this (maybe because of the number of 2x2 reductions)
  #       8 would suggest 2^3 or 3 reductions
  first_conv_filters = mnet.makeDivisible(32*config_model['alpha'], 8)

  image_shape = config_model['image-shape']

  if config_model['nchw']:
    image_shape = (image_shape[2], image_shape[0], image_shape[1])

  inputs = layers.Input(shape=image_shape, name='input_1')

  if config_model['nchw']:
    x = layers.Permute((2,3,1))(inputs)
  else:
    x = inputs

  x = layers.ZeroPadding2D(
    padding=mnet.correctPad(config_model['image-shape'][0], 3, 2),
    name='Conv1_pad')(x)

  in_conv_reg = None
  if config_reg['in-conv']['reg']:
    in_conv_reg = l2(config_reg['in-conv']['reg'])

  x = layers.Conv2D(
    32,
    (3,3),
    strides=(2,2),
    padding='valid',
    use_bias=useBias,
    name='Conv1',
    activity_regularizer=in_conv_reg)(x)

  if not isEval:
    x = mnet.addBN(x, 'Conv1_BN')
  x = mnet.addReLU6(x, 'Conv1_relu')

  if (not isEval) and (not config_reg['in-conv']['dropout'] is None):
    x = mnet.addDropout(x, 'Conv1_dropout', rate=config_reg['in-conv']['dropout'])

  # residual block layers
  # mnet.checkShape(x.shape[1:4], (112,112,32))
  x = mnet.InvertedResidual(x, filters=16, block_id=0, expand=1, stride=1, config=config)
  # mnet.checkShape(x.shape[1:4], (112,112,16))

  x = mnet.InvertedResidual(x, filters=24, block_id=1, expand=6, stride=2, config=config)
  x = mnet.InvertedResidual(x, filters=24, block_id=2, expand=6, stride=1, config=config)
  # mnet.checkShape(x.shape[1:4], (56,56,24))

  x = mnet.InvertedResidual(x, filters=32, block_id=3, expand=6, stride=2, config=config)
  x = mnet.InvertedResidual(x, filters=32, block_id=4, expand=6, stride=1, config=config)
  x = mnet.InvertedResidual(x, filters=32, block_id=5, expand=6, stride=1, config=config)
  # mnet.checkShape(x.shape[1:4], (28,28,32))

  x = mnet.InvertedResidual(x, filters=64, block_id=6, expand=6, stride=2, config=config)
  x = mnet.InvertedResidual(x, filters=64, block_id=7, expand=6, stride=1, config=config)
  x = mnet.InvertedResidual(x, filters=64, block_id=8, expand=6, stride=1, config=config)
  x = mnet.InvertedResidual(x, filters=64, block_id=9, expand=6, stride=1, config=config)
  # mnet.checkShape(x.shape[1:4], (14,14,64))

  x = mnet.InvertedResidual(x, filters=96, block_id=10, expand=6, stride=1, config=config)
  x = mnet.InvertedResidual(x, filters=96, block_id=11, expand=6, stride=1, config=config)
  x = mnet.InvertedResidual(x, filters=96, block_id=12, expand=6, stride=1, config=config)
  # mnet.checkShape(x.shape[1:4], (14,14,96))

  x = mnet.InvertedResidual(x, filters=160, block_id=13, expand=6, stride=2, config=config)
  x = mnet.InvertedResidual(x, filters=160, block_id=14, expand=6, stride=1, config=config)
  x = mnet.InvertedResidual(x, filters=160, block_id=15, expand=6, stride=1, config=config)
  # mnet.checkShape(x.shape[1:4], (7,7,160))

  x = mnet.InvertedResidual(x, filters=320, block_id=16, expand=6, stride=1, config=config)
  # mnet.checkShape(x.shape[1:4], (7,7,320))

  # x = mnet.InvertedResidual(x, filters=1280, block_id=17, expand=6, stride=1)
  # # mnet.checkShape(x.shape[1:4], (7,7,1280))

  # last block conv
  out_conv_reg = None
  if config_reg['out-conv']['reg']:
      out_conv_reg = l2(config_reg['out-conv']['reg'])


  print("useBias: ", useBias)
  x = layers.Conv2D(
    1280,
    (1,1),
    use_bias=useBias,
    name='ConvLast',
    activity_regularizer=out_conv_reg)(x)

  if not isEval:
    x = mnet.addBN(x, 'ConvLast_BN')
  x = mnet.addReLU6(x, 'ConvLast_relu')

  if (not isEval) and (not config_reg['out-conv']['dropout'] is None):
    x = mnet.addDropout(x, 'ConvLast_dropout', rate=config_reg['out-conv']['dropout'])

  # pool
  # replace this with maxpool
  x = layers.GlobalMaxPooling2D(name='global_max_pool')(x)

  # create model
  model_base = keras.Model(inputs=inputs, outputs=x, name="mobilenetv2")

  model_base_out = x

  # load and apply weights from imagenet training
  if config['training']['transfer-learn']:
    weights = loadWeights(alpha=config_model['alpha'], rows=config_model['image-shape'][0])
    applyWeights(model_base, weights)

  # output layer
  dense_reg = None
  if config_reg['dense']['reg']:
      dense_reg = l2(config_reg['dense']['reg'])

  x = layers.Dense(config_model['classes'],
    use_bias=True, name='logits',
    activity_regularizer=dense_reg)(model_base_out)

  x = layers.Softmax(name='softmax')(x)

  model = keras.Model(inputs=inputs, outputs=x)

  print("\n")

  model.summary()

  return model


def txWeights(model_from, model_to):
  # code.interact(local=dict(globals(), **locals()))
  # exit(1)

  for layer in model_from.layers:
    try:
      layer_to = model_to.get_layer(layer.name)
      layer_to.set_weights(layer.get_weights())
    except:
      pass
      


def CreateEvalModel(config=None):
  config['model']['eval'] = True
  config['training']['transfer-learn'] = False
  return CreateModel(config)


def TransferWeights(srcModel, destModel):
  for layer in destModel.layers:
    src_weights = srcModel.get_layer(layer.name).get_weights()
    layer.set_weights(src_weights)


def loadWeights(alpha=1.0, rows=None):
  # https://github.com/JonathanCMitchell/mobilenet_v2_keras/releases/download/v1.1/mobilenet_v2_weights_tf_dim_ordering_tf_kernels_1.0_224_no_top.h5
  model_name = 'mobilenet_v2_weights_tf_dim_ordering_tf_kernels_1.0_224_no_top.h5'
  BASE_WEIGHT_URI='https://github.com/JonathanCMitchell/mobilenet_v2_keras/releases/download/v1.1/'
  weights_path = BASE_WEIGHT_URI + model_name

  weights_file = keras.utils.get_file(model_name, weights_path,
    cache_subdir='weights')

  print(weights_file)

  return weights_file


def applyWeights(model, weights):
  # code.interact(local=dict(globals(), **locals()))
  model.load_weights(weights)


def CompileModel(model, config=None):
  optim = None

  config_train = config['training']

  if config_train['optimizer'] == 'adam':
    # optim = Adam(learning_rate=config['lr'])
    optim = Adam(learning_rate=config_train['lr'])
  if config_train['optimizer'] == 'sgd':
    optim = SGD(lr=config_train['lr'])

  
  model.compile(optimizer=optim,
    loss='categorical_crossentropy',
    metrics=['acc'])


def _lr_decay(epoch, lr):
  return lr * _lr_decay.lr_decay


## Custom Callbacks
class PrintLR(Callback):
  def on_epoch_end(self, epoch, logs=None):
    print(" - lr:{:.2E}".format(backend.eval(self.model.optimizer.lr)), end='')


def CreateCallbacks(config=None):
  cbacks = []
  config_train = config['training']
  save_dir = config['load/save']['dir']

  if config_train['checkpoint']:
    ckpt_path = os.path.join(save_dir, "{epoch}-{val_acc:.2f}.ckpt")
    cbacks.append(ModelCheckpoint(ckpt_path, save_best_only=True))

  if config_train['early-stop']:
    cbacks.append(EarlyStopping(monitor='val_loss', min_delta=0.001, patience=10))

  if config_train['reduce-plateau']:
    cbacks.append(ReduceLROnPlateau(monitor='val_loss', min_lr=config_train['reduce-plateau']['min-lr']))

  return cbacks


## Loading and Saving Model
def FromFile(path):
    return keras.models.load_model(path)

  
def SaveModel(model, config=None):
    save_dir = os.path.join(config['load/save']['dir'], 'model')
    model.save(save_dir)


def SaveFrozenGraphV1(model, config=None):
  # see: https://leimao.github.io/blog/Save-Load-Inference-From-TF-Frozen-Graph/

  pb_dir = os.path.join(config['dir'])
  as_txt = config['as-text']
  
  sess = K.get_session()
  graph = tfv1.get_default_graph()
  input_graph_def = graph.as_graph_def()

  tfv1.graph_util.remove_training_nodes(input_graph_def)  

  output_node_names = [model.output.op.name]
  output_graph_def = tfv1.graph_util.convert_variables_to_constants(sess, input_graph_def, output_node_names)

  layers = [op.name for op in graph.get_operations()]

  pprint.pprint(layers[0])

  # code.interact(local=dict(globals(), **locals()))
  # exit(1)

  tfv1.io.write_graph(output_graph_def, pb_dir, "model.pbtxt", as_text=as_txt)

  # with tf.io.gfile.GFile(pb_filepath, 'wb') as f:
  #     f.write(output_graph_def.SerializeToString())


def SaveForSagemaker(model, config=None):
    sess = K.get_session()
    saved_builder = builder.SavedModelBuilder(os.path.join(config['dir'], "001"))
                         
    signature = predict_signature_def(
    inputs={model.input.name: model.input},
    outputs={model.output.name: model.output})

    # signature = predict_signature_def(
    # inputs={model.input.name[:-2]: model.input},
    # outputs={model.output.name[:-2]: model.output})

    saved_builder.add_meta_graph_and_variables(sess=sess, tags=[SERVING], signature_def_map={"serving_default": signature})
    saved_builder.save()
