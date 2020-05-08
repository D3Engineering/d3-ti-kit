import argparse
import code

import tensorflow as tf
import tensorflow.compat.v1 as tfv1


def _parse_args():
    parser = argparse.ArgumentParser()

    parser.add_argument("model", metavar='model')

    args = parser.parse_args()

    return args

def _load_graph_from_pb(path):
    detection_graph = tfv1.Graph()
    with detection_graph.as_default():
        graph_def = tfv1.GraphDef()
        with tfv1.gfile.GFile(path, 'rb') as fid:
            graph_def.ParseFromString(fid.read())
            tfv1.import_graph_def(graph_def, name='')

            return detection_graph


def _check_pad_conv(layers, i):
    l = layers[i]
    lp = layers[i-1]

    if 'Conv2D' in l:
        if not 'pad' in lp:
            print("Unsupported: {} -> {}".format(lp, l))
        else:
            print("Supported: {} -> {}".format(lp,l))


def _check_ops(layers, i):
    key_words = ['switch', 'merge']

    for k in key_words:
        if k in layers[i]:
            print("Unsupported: {}".format(layers[i]))

            
def _main():
    args = _parse_args()

    graph = _load_graph_from_pb(args.model)
    layers = [op.name for op in graph.get_operations()]
    
    for i in range(len(layers)):
        _check_pad_conv(layers, i)
        _check_ops(layers, i)
        

    code.interact(local=dict(globals(), **locals()))



if __name__ == '__main__':
    _main()
