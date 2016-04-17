all:
	gcc cJSON.c rgb_to_hsv.c home.c -o home -lm
clean:
	rm home
install:
	gcc cJSON.c rgb_to_hsv.c home.c -o home -lm
	cp home /usr/local/bin/home
