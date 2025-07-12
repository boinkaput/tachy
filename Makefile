all: example

all_tests: unit_test time_test

example: example.c src/*.c
	gcc -g \
		-o example \
		example.c src/*.c

unit_test: test/unit_test.c src/*.c
	gcc -DTACHY_TEST -g \
		-o unit_test \
		test/unit_test.c src/*.c

time_test: test/time_test.c src/*.c
	gcc -g \
		-o time_test \
		test/time_test.c src/*.c

clean:
	rm -f example unit_test time_test
