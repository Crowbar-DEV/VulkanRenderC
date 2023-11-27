# VulkanRenderC
Learning vulkan in plain, beautiful(well not my code), C. 

# Build
Currently I build through vscode on my laptop running arch linux. I need to get around to building for windows 
and making it easy to build on linux. If you for some reason want to try and build the project, check tasks.json in 
the .vscode folder to find compiler and linker flags.

## Deps
cglm - Linear algebra library<br>
GLFW - windowing library<br>
Vulkan - rendering<br>

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

# BEWARE
currently this implementation leaks memory -- I am lazy and dont want to trace object lifetimes. Vulkan
Objects are handled appropriately but since C does not have vectors I use flat, heap allocated arrays and 
forgot to free lots of them(oops). I will get around to it eventually but just keep this in mind.