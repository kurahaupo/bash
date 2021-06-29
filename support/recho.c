#include <stdio.h>

int main(int argc, char **argv)
{
	register int	i;

	for (i = 1; i < argc; i++) {
		printf("argv[%d] = <", i);
		strprint(argv[i]);
		printf(">\n");
	}
	return 0;
}

strprint(str)
char	*str;
{
	register char *s;
	int	c;

	for (s = str; s && *s; s++) {
		if (*s < ' ') {
			putchar('^');
			putchar(*s+64);
		} else if (*s == 127) {
			putchar('^');
			putchar('?');
		} else
			putchar(*s);
	}
}
