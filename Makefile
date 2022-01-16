convert:
	mogrify -define bmp:format=bmp3 -type Grayscale -colors 16 -compress none -format bmp 14/**/*.png