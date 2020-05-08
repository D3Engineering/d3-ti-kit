#!/bin/bash

in_graph=$1
in_node=$2
out_node=$3

/home/qzpv/docs/projects/aws-ti/tvm_demo/tensorflow/bazel-bin/tensorflow/tools/graph_transforms/transform_graph \
--in_graph=$in_graph \
--out_graph=${in_graph}.strip \
--inputs="$in_node" \
--outputs="$out_node" \
--transforms='
  strip_unused_nodes(type=float, shape="4,3,224,224")
  remove_nodes(op=Identity, op=CheckNumerics)
  fold_constants(ignore_errors=true)
  fold_batch_norms
  fold_old_batch_norms'
