all_tests: unit_test time_test

unit_test: test/unit_test.c src/*.c
	gcc -DTACHY_TEST -g \
		-o unit_test \
		test/unit_test.c src/*.c

time_test: test/time_test.c src/*.c
	gcc -DTACHY_TEST -g \
		-o time_test \
		test/time_test.c src/*.c

clean:
	rm -f unit_test time_test
