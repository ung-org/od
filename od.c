#define _XOPEN_SOURCE 700
#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

union block {
	char c[16];
	short s[16 / sizeof(short)];
	unsigned short us[16 / sizeof(unsigned short)];
	int i[16 / sizeof(int)];
	unsigned int ui[16 / sizeof(unsigned int)];
	long l[16 / sizeof(long)];
	unsigned long ul[16 / sizeof(unsigned long)];
	long long ll[16 / sizeof(long long)];
	unsigned long long ull[16 / sizeof(unsigned long long)];
	float f[16 / sizeof(float)];
	double d[16 / sizeof(double)];
	long double ld[16 / sizeof(long double)];
};

static int escapes = 0;
static int verbose = 0;

static int addoutput(const char *type)
{
	(void)type;
	return 0;
}

static int od(const char *filename, char address_base, unsigned long skip, unsigned long skipsize, unsigned long maxbytes)
{
	FILE *in = stdin;
	union block prev;
	union block b;
	size_t nread = 0;
	int star = 0;
	char basefmt[] = { '%', '0', '7', 'z', address_base, ' ', '\0' };

	if (strcmp(filename, "-")) {
		in = fopen(filename, "rb");
		if (in == NULL) {
			fprintf(stderr, "od: couldn't open %s: %s\n", filename, strerror(errno));
			return 1;
		}
	}

	if (skip > skipsize) {
		unsigned long tmp = skip;
		skip = skipsize;
		skipsize = tmp;
	}

	for (unsigned long i = 0; i < skip; i++) {
		fseek(in, skipsize, SEEK_SET);
		nread += skipsize;
	}

	while ((fread(&b, sizeof(b), 1, in)) == 1) {
		nread += sizeof(b);
		if (maxbytes && nread >= maxbytes) {
			break;
		}

		if (!verbose && memcmp(&prev, &b, sizeof(b)) == 0) {
			if (!star) {
				printf("*\n");
				star = 1;
			}
			continue;
		}

		star = 0;

		if (address_base != 'n') {
			printf(basefmt, nread - sizeof(b));
		}

		/* TODO: this loop */
		size_t nloops = sizeof(b.s) / sizeof(b.s[0]);
		for (size_t i = 0; i < nloops; i++) {
			printf("%06ho%c", b.s[i], i == nloops - 1 ? '\n' : ' ');
		}

		memcpy(&prev, &b, sizeof(b));
	}

	return 0;
}

int main(int argc, char *argv[])
{
	char *endopt = NULL;
	unsigned long skip = 0;
	unsigned long maxbytes = 0;
	unsigned long skipsize = 1;
	int xsi_offset = 1;
	char address_base = 'o';
	int c;

	setlocale(LC_ALL, "");

	while ((c = getopt(argc, argv, "A:bcdj:Nost:vx")) != -1) {
		switch (c) {
		case 'A':	/* address_base */
			xsi_offset = 0;
			if (strlen(optarg) != 1) {
				return 1;
			}
			address_base = *optarg;
			if (address_base != 'd' && address_base != 'o' && address_base != 'x' && address_base != 'n') {
				return 1;
			}
			break;

		case 'b':
			addoutput("o1");
			break;

		case 'c':
			escapes = 1;
			break;

		case 'd':
			addoutput("u2");
			break;

		case 'j':	/* skip */
			xsi_offset = 0;
			skip = strtoul(optarg, &endopt, 0);
			switch (*endopt) {
			case 'b':
				skipsize = 512;
				break;

			case 'k':
				skipsize = 1024;
				break;

			case 'm':
				skipsize = 1024 * 1024;
				break;

			case '\0':
				break;

			default:
				fprintf(stderr, "od: unknown suffix '%c'\n", *endopt);
				return 1;
			}
			break;

		case 'N':	/* count */
			xsi_offset = 0;
			maxbytes = strtoul(optarg, NULL, 0);
			break;

		case 'o':
			addoutput("o2");
			break;

		case 's':
			addoutput("d2");
			break;

		case 't':	/* type_string */
			xsi_offset = 0;
			if (addoutput(optarg) != 0) {
				return 1;
			}
			break;

		case 'v':
			xsi_offset = 0;
			verbose = 1;
			break;

		case 'x':
			addoutput("x2");
			break;

		default:
			return 1;
		}
	}

	if (xsi_offset) {
		/* check argv[argc-1] */
		char *xo = "";
		int base = 8;
		size_t len = strlen(xo);

		/* ignore leading '+' */
		if (*xo == '+') {
			xo++;
			len--;
		}

		if (len > 1 && xo[len-1] == 'b') {
			skipsize = 512;
			xo[len-1] = '\0';
			len--;
		}

		if (len > 1 && xo[len-1] == '.') {
			base = 10;
			xo[len-1] = '\0';
		}

		skip = strtoul(argv[argc-1], NULL, base);
	}

	int ret = 0;
	do {
		ret |= od(argv[optind++], address_base, skip, skipsize, maxbytes);
	} while (argv[optind]);

	return ret;
}
