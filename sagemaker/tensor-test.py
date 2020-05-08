#!/usr/bin/env python3

import tensorflow as tf
from tensorflow import keras
from tensorflow.keras import layers, utils
from tensorflow.keras.optimizers import SGD
# import tensorflow.compat.v1 as tfv1
import numpy as np
import tensorflow.keras.backend as K

import argparse
import os
import code
import pprint

def makeDivisible(num, div):
  return int(num // div) * div


# copied from https://github.com/keras-team/keras-applications/blob/master/keras_applications/__init__.py
# and modified
def correctPad(img_size, kernel_size, stride):
    adjust = (1 - img_size % 2, 1 - img_size % 2)

    correct = (kernel_size // 2, kernel_size // 2)

    return ((correct[0] - adjust[0], correct[0]),
            (correct[1] - adjust[1], correct[1]))


def addBN(x, name, axis=-1):
#   return x
  return layers.BatchNormalization(
    axis=axis,
    epsilon=1e-3,
    momentum=0.999,
    fused=True,
    name=name)(x)


def addReLU6(x, name):
  return layers.ReLU(6., name=name)(x)


def InvertedResidual(x,
  filters=None, stride=None,
  expand=1, alpha=1.0,
  block_id=None,config=None):

  inputs = x
  input_channels = inputs.shape[-1]
  exp_channels = int(input_channels * expand)
  filters *= alpha


  prefix = 'block_{}_'.format(block_id)

  if block_id != 0:
    # expand

    x = layers.Conv2D(
      exp_channels,
      (1,1),
      padding='same',
      use_bias=False,
      activation=None,
      name=prefix + 'expanded')(x)

    x = addBN(x, prefix + 'expand_BN')
    x = addReLU6(x, prefix + 'expand_relu')

  # depthwise
  strides = (stride,stride)

  if stride == 2:
    # code.interact(local=dict(globals(), **locals()))
    # exit(1)
    x = layers.ZeroPadding2D(
      padding=correctPad(x.shape[1], 3, stride),
      name=prefix + 'pad')(x)

  # single filter full depth convolution
  x = layers.DepthwiseConv2D(
    (3,3),
    use_bias=False,
    activation=None,
    strides=strides,
    padding='same' if stride==1 else 'valid',
    name=prefix + 'depthwise')(x)

  x = addBN(x, prefix + 'depth_BN')
  x = addReLU6(x, prefix + 'depthwise_relu')


  # project - multi-filter 1x1 convolution
  filters = makeDivisible(filters, 8)

  x = layers.Conv2D(
    filters,
    (1,1),
    padding='same',
    use_bias=False,
    activation=None,
    name=prefix + 'project')(x)

  x = addBN(x, prefix + 'project_BN')


  # skip connection, if possible
  if stride == 1 and input_channels == filters:
    x = layers.Add(name=prefix + 'add')([inputs, x])

  return x


def _parse_args():
    parser = argparse.ArgumentParser()

    parser.add_argument('--model_dir', type=str)
    parser.add_argument('--sm-model-dir', type=str, default=os.environ.get('SM_MODEL_DIR'))

    return parser.parse_args()


def SaveFrozenGraphV1(model, out_dir):
  # see: https://leimao.github.io/blog/Save-Load-Inference-From-TF-Frozen-Graph/
  import tensorflow
  tf = tensorflow
  
  try:
    tf=tf.compat.v1
  except:
    pass

  pb_filepath = os.path.join(out_dir, 'model.pb')
  
  sess = K.get_session()
  graph = tf.get_default_graph()
  input_graph_def = graph.as_graph_def()

  code.interact(local=dict(globals(), **locals()))

  output_node_names = [model.output.op.name]
  output_graph_def = tf.graph_util.convert_variables_to_constants(sess, input_graph_def, output_node_names)

  layers = [op.name for op in graph.get_operations()]

  pprint.pprint(layers[0])

  with tf.gfile.GFile(pb_filepath, 'wb') as f:
      f.write(output_graph_def.SerializeToString())
    

def SaveForSagemaker(model, out_dir):
    saved_model = None
    signature_def_utils = None
    
    try:
        from tensorflow.compat.v1 import saved_model as sm
        saved_model = sm
       
        from tensorflow.compat.v1.saved_model import signature_def_utils as sig_def
        signature_def_utils = sig_def
    except:
        from tensorflow import saved_model as sm
        saved_model = sm
       
        from tensorflow.saved_model import signature_def_utils as sig_def
        signature_def_utils = sig_def
        
        
    sess = K.get_session()
    saved_builder = saved_model.builder.SavedModelBuilder(os.path.join(out_dir, "001"))

    signature = signature_def_utils.predict_signature_def(
    inputs={model.input.name: model.input},
    outputs={model.output.name: model.output})

    saved_builder.add_meta_graph_and_variables(sess=sess, tags=[saved_model.SERVING], signature_def_map={"serving_default": signature})
    saved_builder.save()


def _main():
    args = _parse_args()

#     a = tf.placeholder(tf.float32, shape=[2, 2], name="input1")
#     b = tf.placeholder(tf.float32, shape=[2, 2], name="input2")
#     ab = tf.matmul(a, b)

    inputs = layers.Input(shape=(224,224,3), batch_size=1, name='input_1')

    x = layers.ZeroPadding2D(
      padding=correctPad(224, 3, 2),
      name='Conv1_pad')(inputs)
    
    x = layers.Conv2D(
      32,
      (3,3),
      strides=(2,2),
      padding='valid',
      use_bias=False,
      name='Conv1')(x)
  
    x = addBN(x, 'Conv1_BN')
    x = addReLU6(x, 'Conv1_relu')
  
    x = InvertedResidual(x, filters=16, block_id=0, expand=1, stride=1)

    x = layers.Conv2D(
      1280,
      (1,1),
      use_bias=False,
      name='ConvLast')(x)
    x = addBN(x, 'ConvLast_BN')
    x = addReLU6(x, 'ConvLast_relu')
    
    x = layers.GlobalAveragePooling2D(name='global_avg_pool')(x)

    x = layers.Dense(2,
      use_bias=True, name='logits')(x)

    x = layers.Softmax(name='softmax')(x)
                            
    model = keras.Model(inputs=inputs, outputs=x)
    model.compile(optimizer=SGD(lr=0.001),
        loss='categorical_crossentropy', metrics=['acc'])
    model.summary()
    

    X_train = np.random.random((10,224,224,3))
    y_train = utils.to_categorical(np.random.randint(0, high=2, size=(10,)))
    
    model.fit(
      X_train[0:9], y_train[0:9],
      epochs=1, batch_size=1,
      validation_data=(X_train[9:], y_train[9:]))
    
#     SaveForSagemaker(model, args.sm_model_dir)
    SaveFrozenGraphV1(model, args.sm_model_dir)
    

if __name__ == '__main__':
    _main()