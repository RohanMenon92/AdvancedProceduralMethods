## Advanced Procedural Methods Coursework
Implementation of the marching cubes algorithm along with a KD tree for collision detection. 

I'm also rendering a real time shadow map using render texture. As well as texturing the objects using tri planar displacement mapping for slope shading. 
To showcase the TriPlanar mapping, I am using a grass, rock and brick texture.

The terrain can be modified by defining different terrain presets, and by defining the resolution of the 3D grid rendering the marching cubes algorithm.

The large scale terrain on which the sample is built is also created using the marching cubes algorithm.

A KD Tree and raycasting is used for collision detection.

Video Link: https://youtu.be/3PMkE95prCM

## HLSL Shaders
Implementation is done in DirectX (C++) using 4 different kinds of HLSL shaders:
- Pixel Shaders (TriPlanar displacement, Shadow, Skydome)
- Vertex Shaders (TriPlanar Displacement, Shadow, Marching Cubes, Skydome)
- Geometry Shader (Marching Cubes Geometry)
- HULL Shaders (Tesselation)

## Controls
- W, A, S, D to move
- Mouse to look
- Foldable IMGUI panels for controlling terrain, KD tree, tesselation rendering and collision detection

## Screenshots
![Noise Generation 1](https://github.com/RohanMenon92/AdvancedProceduralMethods/blob/master/Screenshots/Capture2.PNG)
![Noise Generation 2](https://github.com/RohanMenon92/AdvancedProceduralMethods/blob/master/Screenshots/MainCapture.PNG)
![Movement and KDTree Controls](https://github.com/RohanMenon92/AdvancedProceduralMethods/blob/master/Screenshots/KDTree.PNG)
![Terrain Generation Control](https://github.com/RohanMenon92/AdvancedProceduralMethods/blob/master/Screenshots/TerrainObjectControl.PNG)
![Tesselation](https://github.com/RohanMenon92/AdvancedProceduralMethods/blob/master/Screenshots/Tesselation.PNG)

