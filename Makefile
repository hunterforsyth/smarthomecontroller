all:
	gcc home.c -o home
clean:
	rm home
install:
	gcc home.c -o home
	cp home /usr/local/bin/home