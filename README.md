# glTF Viewer DX
### A viewer of the glTF file format, implemented in DirectX 12 

This project aims to realize a software that can load and render files written according to the [glTF](https://github.com/KhronosGroup/glTF/tree/master/specification/2.0) specification. The software is implemented in C++, DirectX 12 and HLSL.

[glTF](https://github.com/KhronosGroup/glTF/tree/master/specification/2.0) is a specification from KhronosGroup for a file format that can describe:
* 3D geometry
* materials, described according to a [shading model](https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#implementation) defined in the glTF specification
* animations

The viewer is also able to edit the vertex, geometry and fragment shaders on the fly, to see how the shader code affects the model.

glTF Viewer is an ongoing project actually under development. Check out the [project roadmap](https://github.com/salvatorespoto/GLTFViewer/projects/1)

### Current supported features are:

* Scenes: scene, nodes hierarchy, node transformation
* Meshes: geometry, all the attributes are supported (position, normal, tangent, textcoord_0, etc.), instantiation
* Materials: textures, images, samples, additional maps (normal, occlusion, emission)
* Shading model

### Unsupported (yet) features
* Ray Tracing
* Sparse accessors 
* Animations 
* Skinning
* Morphing
* Cameras
* Interpolation

### Click on the image will show a short video of the application.

[![A video of the application:](http://i3.ytimg.com/vi/tEVuwpKdP4A/maxresdefault.jpg)](https://www.youtube.com/watch?v=tEVuwpKdP4A)
