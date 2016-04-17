all:
	gcc cJSON.c home.c -o home -lm
clean:
	rm home
install:
	gcc cJSON.c home.c -o home -lm
	cp home /usr/local/bin/home
