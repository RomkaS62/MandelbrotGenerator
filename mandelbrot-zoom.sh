#!/bin/sh

width=800
height=640
frames=120
initial_zoom=0
final_zoom=15
smoothing=0.1
multisample=0
directory="./mandelbrot-animation"
real=0.3
imaginary=0.7
iterations=1000
threads=4

help="
--width -w:             Witdth of frames in pixels.
--height -h             Height of frames in pixels.
--real -x               Real coordinate of point to zoom into.
--imaginary -y          Imaginary coordinate of point to zoom into.
--initial-zoom          Initial zoom of first frame. Power of -10.
--final-zoom            Final zoom of the last frame. Power of -10.
--smoothing -s          How much camera movement is smoothed during rendering.
--iterations            Number of attempts before giving up and coloring a
                        pixel black.
--multisample -m        Multisampling level. Think long and hard before setting
                        it to anthing other than 0 as it eats performance for
                        breakfast.
--directory             Directory to put the frames into.
--threads -t            Number of threads to use for rendering.
--frames -f             Number of frames to render.
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
		--imaginary|-y)
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
		--threads|-t)
			shift 1
			threads="$1"
			;;
		--iterations)
			shift 1
			iterations="$1"
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
bp1="0 $initial_zoom"
bp2="1 $final_zoom"
cp1="$smoothing $initial_zoom"
cp2="$smoothing_end $final_zoom"
zoom_bezier="$bp1 $cp1 $bp2 $cp2"
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

if [ ! -d "$directory" ] && [ ! -e "$directory" ]
then
	echo "Creating directory $directory"
	mkdir "$directory"
fi

i=0

for radius in `bezier $zoom_bezier $frames | awk '{ print 1 / 10 ** $2 }'`
do
	mandelbrot -x "$real" -y "$imaginary" -w "$width" -h "$height" -r "$radius" -a "$iterations" -t "$threads" -s "$multisample" -f "$directory/${i}_frame.bmp" > /dev/null
	wait
	echo "$((i + 1)) / $frames"
	i=$((i+1))
done
