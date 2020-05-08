import copy
import code
import json
import os
import datetime as dt

def _dictIter(d, keys):
    for k in d:
        keys.append(k)
        if isinstance(d[k], dict):
            # print("node {} is dict".format(k))
            yield from _dictIter(d[k], keys)
        else:
            # print("node {} is not dict".format(k))
            yield d[k], keys
        keys.pop()

def _dictSet(d, keys, val):
    subdict = None
    
    for k in keys[:-1]:
        if subdict:
            subdict = subdict[k]
        else:
            subdict = d[k]
    
        subdict[keys[-1]] = val

def _hash_config(config):
    config_json = json.dumps(config, sort_keys=True)
    config['hash'] = dt.datetime.now().strftime("%y-%m-%d_%H:%M:%S")

    export_dir = "{}-".format(config['prefix'])
    export_dir += "{}_".format(config['training']['optimizer'])

    if config['training']['early-stop']:
        export_dir += "_estop"
    if config['training']['reduce-plateau']['enabled']:
        export_dir += "_rplat"
    if config['data']['augment']['enabled']:
        export_dir += "_aug"
    
    export_dir += "_{}".format(config['hash'])
    config['load/save']['dir'] = os.path.join(config['load/save']['dir'], export_dir)

    return config_json


def GenConfigs():
    global_reg = 1e-7

    config = {
        'prefix': 'mobilenetv2',
        'hash': '',
        'local': False,
        
        'data': {
            'path': './data/jkiggins_lens_screw',
            'train': 0.6,
            'valid': 0.2,
            'test': 0.2,

            'augment': {
                'enabled': [True],
                'translate-x': [True],
                'translate-y': [True],
                'hflip': [True],
                'vflip': [True],
            },

            'gan-augment': {
                'enabled': [False]
            }
        },

        'training':
        {
            'epochs': [100],
            'optimizer': ['sgd'],
            'lr': 0.0001,
            'batch-size': [32],
            'transfer-learn': True,

            'checkpoint': True,
            'early-stop': [False],
            'reduce-plateau': {
                'enabled': [True],
                'min-lr': 1e-6
            }
        },

        'model': {
            'alpha': 1.0,
            'image-shape': (224,224,3),
            'eval': False,
            'eval-batch-size': 4,
            'classes': 2,
            'nchw': True,

            'regularize': {
                'in-conv': {
                    'reg': [None],
                    'dropout': [None]
                },
                'inv-layers':{
                    'expand': {
                        'reg': [None],
                        'dropout': [None]
                    },
                    'depthwise': {
                        'reg': [None],
                        'dropout': [None]
                    },
                    'project': {
                        'reg': [None],
                        'dropout': [None]
                    }
                },
                'out-conv':{
                    'reg': [global_reg],
                    'dropout': [None]
                },
                'dense':{
                    'reg': [global_reg],
                }
            }
        },

        'load/save':
        {
            'dir': 'artifacts',
            'as-text': False
        }   
    }

    keys_list = []
    list_values = []
    num_configs = 0

    for node, keys in _dictIter(config, []):
        if isinstance(node, list):
            # print("keys: ", keys)
            keys_list.append(keys.copy())
            list_values.append(node)

            if num_configs != 0 and num_configs != len(node):
                raise ValueError("All config lists must have the same length")
            num_configs = len(node)


    # code.interact(local=dict(globals(), **locals()))
    # exit(1)

    for i in range(num_configs):
        for j in range(len(keys_list)):
            _dictSet(config, keys_list[j], list_values[j][i])
        
        yield config


def SaveConfig(config, postfix=None):
    if not config['local']:
        return

    _hash_config(config)

    if postfix:
        config['load/save']['dir'] += "_{}".format(postfix)

    if os.path.exists(config['load/save']['dir']):
        raise ValueError("{} already exists".format(config['load/save']['dir']))
    os.mkdir(config['load/save']['dir'])

    with open(os.path.join(config['load/save']['dir'], 'config.json'), 'w') as f:
        json.dump(config, f)


if __name__ == '__main__':
    from pprint import pprint

    for i, config in enumerate(GenConfigs()):
        print("config ", i)
        pprint(config)
