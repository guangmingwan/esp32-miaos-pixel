.PHONY: all clean manifest

all:
	idf.py build

manifest: all
	python ../../../tools/append_manifest.py --input build/lava_pal.bin --category Games --name lava_pal

clean:
	idf.py fullclean
