# GPU Emulation Stress Test

![gpu-emulation-stree-test-image](gpu_emulation_stress_test.gif)

In graphics for virtual machines, one potential way to get good performance is
to use the host's GPU hardware directly. This can involve the serialization of
all graphics API calls (OpenGL, Vulkan, etc) from/to guest and host, because
the guest usually resides in a different memory address space from the host and
also the guest/host CPU ISA's often differ.

Such communication channels can suffer from overhead when there are many API
commands issued per second.  The GPU Emulation Stress Test measures the
performance of graphics virtualization in the form of an Android NDK app that
runs in the guest. It stresses the communication channel by issuing many draw
calls per second, ramping up from just a few, to thousands per second (default
~1000 objects -> ~2 draw calls per object, one for shadow map writing and
another for the final color -> ~2000 draw calls per frame).

The number of objects can be adjusted to observe scaling behavior. There is
also some degree of fillrate testing depending on the API level used, with the
OpenGL ES 3.0 version stressing fillrate more due to more fullscreen effects:

- OpenGL ES 2.0: Bilinear-filtered shadow map with simple diffuse lighting.
- OpenGL ES 3.0: Motion blur and soft shadows with exponential shadow maps.
