# A 3D Engine
Some goofy engine I wrote with the scraps of the 2.5D engine.

## Requirements for finished product
* A little endian machine (probably not like people are rolling with RISC-Vs)
* OpenGL or Vulkan (probably)

## Requirements for development
* python 3.0+
* python trimesh, pillow, and numpy
* cmake
* gcc/clang
* SDL3

## Windows
Compiling for Windows sucks. Use mingw if you don't want to lose your mind.
SDL will also have to linked when generating cmake files so good luck.

## Misc
* This engine has no depth buffer. Can be approximated by adding a depth variable on the faces of a mesh.
* No scene culling (yet.)
* Texture mapping does not exist. Too lazy for that.
* Will be Unix first because testing this on Windows has lost me hairs.

## Also see:
* 2D Engine: [![GitHub](https://img.shields.io/badge/github-%23121011.svg?style=logo&logo=github&logoColor=white)](https://github.com/SignificantRichard/2D-Engine)

![Build Status]( https://img.shields.io/badge/build-working-green?style=for-the-badge) ![Development]( https://img.shields.io/badge/Development-ongoing-green?style=for-the-badge) ![Caffine Status]( https://img.shields.io/badge/Caffine-ran_out-red?style=for-the-badge)

> <hr><br>2026 Wen Xin Gao.<br><br><br>