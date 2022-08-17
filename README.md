# Fractalgen

This program draws fractal images. Fractal generating functions are to be
chosen from plugins in the form of shared objects chosen via a glob pattern
specified by environment variable FRACTALGEN\_PLUGIN\_PATTERN. Default plugins
are installed in /usr/local/lib/fractalgen. In this case set the pattern to
/usr/local/lib/fractalgen/\*.so.

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

## Usage

```[sh]
$ fractalgen -x 0 -y 0 -r 1.5 -f image.bml -w 1920 -h 1080 --iterate mandelbrot-double --render render-rgb -a 10000 -t 4 -s 0
```

	*	-x: x coordinate of center of viewport.
	*	-y: y coordinate of center of viewport.
	*	-r: Viewport radius along the smaller dimension.
	*	-w: Image width in pixels.
	*	-h: Image height in pixels.
	*	-f: Filename under which the image shall be saved.
	*	-t: Number of threads to use for the operation.
	*	-s: Supersample level. Uses 2^n more pixels to render the final image.  I recomment against using more than than 2.
	*	--iterate: Function to iterate. Defaults to 'mandelbrot-double'.
	*	--render: Name of the function that will convert samples from iterate into RGB pixels. Defaults to 'render-rgb'.

## Plugin: How to?

Write a shared library that exports a 'const struct fractal\_iterator\_s iterators'.
