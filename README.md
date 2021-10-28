# MandelbrotGenerator

This repository contains two programs:

  1. A command line program that generates images of the mandelbrot set.
  1. A Java GUI that makes it actually usable.

If you want to use this, make sure that you build mandelbrot CLI program
and put the executable in your PATH. The GUI uses the CLI program under
the hood to generate images. GUI requires no special installation to work.

## How to build and install

```[sh]
$ mkdir build
$ cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/
$ make -j4
$ sudo make install
```

## How to uninstall

```[sh]
$ cd build
$ sudo xargs < install_manifest.txt rm
```
