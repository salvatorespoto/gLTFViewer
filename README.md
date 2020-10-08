# glTF Viewer DX
## A viewer of the glTF file format, implemented in DirectX 12 

The aim of this project is to realize a program that can load and render files written accorting to the [glTF](https://github.com/KhronosGroup/glTF/tree/master/specification/2.0) specification. The software is implemented in C++, DirectX 12 and HLSL.

[glTF](https://github.com/KhronosGroup/glTF/tree/master/specification/2.0) is a specification from KhronosGroup that that describes a file format that is able to describe:
* 3D geometry
* materials, described according a [shading model](https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#implementation) defined in the gltf specification
* animations

The viewer is also able to edit the vertex, geometry and fragment shaders on the fly, to see how the shader code affects the model.

This is an ongoing project actually under development. 

## Current supported feature are:

* Scenes: scene, nodes hierarchy, node transformation
* Meshes: geometry, all the attributes are supported (position, normal, tangent, textcoord_0, etc.), instantiation
* Materials: textures, images, samples, additional maps (normal, occlusion, emission)
* Shading model

## Unsuppoted (yet) features
* Sparse accessors 
* Animations 

### Click on the image will show a short video of the application.

[![A video of the application:](http://i3.ytimg.com/vi/tEVuwpKdP4A/maxresdefault.jpg)](https://www.youtube.com/watch?v=tEVuwpKdP4A)
