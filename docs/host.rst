.. _host_ref:

####
Host
####

Overview
========
The host directory consists of compile and setup scripts which can take a
tensorflow exported model, and compile it using TVM. Neo is being used
primarily for compiling at this point so maybe the host code should be
abandoned. However the Neo runtime is able to load and run tvm-compiled models,
so maybe the customer will still want this.
