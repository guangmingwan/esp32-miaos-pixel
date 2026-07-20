.PHONY: all clean manifest sdl app

MIA_SDL_DIR := ../../../shared_libraries/mia_sdl

all: app

sdl:
	cd $(MIA_SDL_DIR) && idf.py so

app:
	idf.py build

manifest: app
	python ../../../tools/append_manifest.py --input build/lava_pal.bin --category Games --name lava_pal

clean:
	idf.py fullclean
