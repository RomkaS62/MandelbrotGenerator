#!/bin/sh

width=800
height=640
frames=120
initial_zoom=1
final_zoom=0.0001
smoothing=0.1
multisample=0
directory="./mandelbrot-animation"
real=0.3
imaginary=0.7

help="
--width -w:             Witdth of frames in pixels.
--height -h             Height of frames in pixels.
--real -x               Real coordinate of point to zoom into.
--imaginary -y          Imaginary coordinate of point to zoom into.
--initial-zoom          Initial zoom of first frame. Power of -10.
--final-zoom            Final zoom of the last frame. Power of -10.
--smoothing -s          How much camera movement is smoothed during rendering.
--multisample -m        Multisampling level. Think long and hard before setting
                        it to anthing other than 0 as it eats performance for
                        breakfast.
--directory             Directory to put the frames into.
--frames -f             Number of frames to rendef.
--help                  Prints this help text and exits.
"

while [ $# -gt 0 ]
do
	case "$1" in
		--width|-w)
			shift 1
			width="$1"
			;;
		--height|-h)
			shift 1
			height="$1"
			;;
		--real|-x)
			shift 1
			real="$1"
			;;
		--imaginary|-i)
			shift 1
			imaginary="$1"
			;;
		--multisample|-m)
			shift 1
			multisample="$1"
			;;
		--initial-zoom)
			shift 1
			initial_zoom="$1"
			;;
		--final-zoom)
			shift 1
			final_zoom="$1"
			;;
		--smoothing|-s)
			shift 1
			smoothing="$1"
			;;
		--directory|-d)
			shift 1
			directory="$1"
			;;
		--frames|-f)
			shift 1
			frames="$1"
			;;
		--help|-h)
			echo "$help"
			exit 0
			;;
		*)
			echo "Unrecognised argument \"$1\""
			exit 1;
			;;
	esac
	shift 1
done

smoothing_end=`awk "BEGIN {print 1 - $smoothing}"`
zoom_bezier="0 $initial_zoom $smoothing $initial_zoom $smoothing_end $final_zoom  1 $final_zoom"

echo "
Zoom bezier:
        [0, $initial_zoom]
        [$smoothing, 0]
        [$smoothing_end, $final_zoom]
        [1, $final_zoom]
"

echo "Width:                    $width"
echo "Height:                   $height"
echo "Frames:                   $frames"
echo "Camera bezier:            $zoom_bezier"
echo "Multisample level:        $multisample"
echo "Directory:                $directory"

if [ -f "$directory" ]
then
	echo "Fiile $directory already exists"
fi

if [ ! -d "$directory" ] && [ ! -e ]
then
	mkdir "$directory"
fi

i=0

for radius in `bezier $zoom_bezier $frames y | awk '{ print 1 / 10 ** $1 }'`
do
	mandelbrot -x "$real" -y "$imaginary" -w "$width" -h "$height" -r "$radius" -a 1000 -t 4 -s "$multisample" -f "$directory/${i}_frame.bmp" &> /dev/null
	wait
	echo "$((i + 1)) / $frames"
	i=$((i+1))
done
