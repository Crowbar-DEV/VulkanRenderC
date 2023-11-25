# VulkanRenderC
Trying to use vulkan with C, idk why, just bad at C++, using cglm for linear algebra https://github.com/recp/cglm
Currently Using GLFW for windowing. I am at the vertex buffer part of the vulkan tutorial. But we got a triangle to render, hell yeah!

# Docs
if interested, I made a little shell script to generate
documentation for vulkan functions and structures,
if you want to add documentation for a function or structure
just add it to docs/neededApiCalls.txt or docs/neededApiStructs.txt respectively
then run
```
bash gendocs.sh
```
documentation will be output to docs/functions docs/structs respectively
