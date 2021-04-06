shell:
	shell.c
	gcc -Wall -std=c99 -o shell shell.c -D _XOPEN_SOURCE=500 -D _POSIX_C_SOURCE

clean:
	rm -f shell
