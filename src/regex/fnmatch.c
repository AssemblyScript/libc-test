#include <stdio.h>
#include <stdlib.h>
#include <fnmatch.h>
#include <unistd.h>
#include "test.h"

/* adapted from dietlibc's test-newfnmatch.c */

/* xlat / printflags adapted from http://www.liacs.nl/~wichert/strace/ */
#define FLAG(f) { f, #f }

struct xlat {
	int val;
	char *str;
} fnmatch_flags[] = {
	FLAG(FNM_NOESCAPE),
	FLAG(FNM_PATHNAME),
	FLAG(FNM_PERIOD),
	{0, NULL},
};

static void printflags(const struct xlat *map, int flags) {
	char *sep;

	if (!flags) {
		fprintf(stderr, "0");
		return;
	}
	sep = "";
	for (; map->str; map++) {
		if (map->val && (flags & map->val) == map->val) {
			fprintf(stderr, "%s%s", sep, map->str);
			sep = "|";
			flags &= ~(map->val);
		}
	}
	if (flags) fprintf(stderr, "%sunknown=%#x", sep, flags);
}

/* tests harness adapted from glibc testfnm.c */
struct {
	const char *pattern;
	const char *string;
	int flags;
	int expected;
} tests[] = {
	/* begin dietlibc tests */
	{ "*.c", "foo.c", 0, 0 },
	{ "*.c", ".c", 0, 0 },
	{ "*.a", "foo.c", 0, FNM_NOMATCH },
	{ "*.c", ".foo.c", 0, 0 },
	{ "*.c", ".foo.c", FNM_PERIOD, FNM_NOMATCH },
	{ "*.c", "foo.c", FNM_PERIOD, 0 },
	{ "a\\*.c", "a*.c", FNM_NOESCAPE, FNM_NOMATCH },
	{ "a\\*.c", "ax.c", 0, FNM_NOMATCH },
	{ "a[xy].c", "ax.c", 0, 0 },
	{ "a[!y].c", "ax.c", 0, 0 },
	{ "a[a/z]*.c", "a/x.c", FNM_PATHNAME, FNM_NOMATCH },
	{ "a/*.c", "a/x.c", FNM_PATHNAME, 0 },
	{ "a*.c", "a/x.c", FNM_PATHNAME, FNM_NOMATCH },
	{ "*/foo", "/foo", FNM_PATHNAME, 0 },
	{ "-O[01]", "-O1", 0, 0 },
	{ "[[?*\\]", "\\", 0, 0 },
	{ "[]?*\\]", "]", 0, 0 },
	/* initial right-bracket tests */
	{ "[!]a-]", "b", 0, 0 },
	{ "[]-_]", "^", 0, 0 }, /* range: ']', '^', '_' */
	{ "[!]-_]", "X", 0, 0 },
	{ "??", "-", 0, FNM_NOMATCH },
	/* begin glibc tests */
	{ "*LIB*", "lib", FNM_PERIOD, FNM_NOMATCH },
	{ "a[/]b", "a/b", 0, 0 },
	{ "a[/]b", "a/b", FNM_PATHNAME, FNM_NOMATCH },
	{ "[a-z]/[a-z]", "a/b", 0, 0 },
	{ "*", "a/b", FNM_PATHNAME, FNM_NOMATCH },
	{ "*[/]b", "a/b", FNM_PATHNAME, FNM_NOMATCH },
	{ "*[b]", "a/b", FNM_PATHNAME, FNM_NOMATCH },
	{ "[*]/b", "a/b", 0, FNM_NOMATCH },
	{ "[*]/b", "*/b", 0, 0 },
	{ "[?]/b", "a/b", 0, FNM_NOMATCH },
	{ "[?]/b", "?/b", 0, 0 },
	{ "[[a]/b", "a/b", 0, 0 },
	{ "[[a]/b", "[/b", 0, 0 },
	{ "\\*/b", "a/b", 0, FNM_NOMATCH },
	{ "\\*/b", "*/b", 0, 0 },
	{ "\\?/b", "a/b", 0, FNM_NOMATCH },
	{ "\\?/b", "?/b", 0, 0 },
	{ "[/b", "[/b", 0, -FNM_NOMATCH },
	{ "\\[/b", "[/b", 0, 0 },
	{ "??""/b", "aa/b", 0, 0 },
	{ "???b", "aa/b", 0, 0 },
	{ "???b", "aa/b", FNM_PATHNAME, FNM_NOMATCH },
	{ "?a/b", ".a/b", FNM_PATHNAME|FNM_PERIOD, FNM_NOMATCH },
	{ "a/?b", "a/.b", FNM_PATHNAME|FNM_PERIOD, FNM_NOMATCH },
	{ "*a/b", ".a/b", FNM_PATHNAME|FNM_PERIOD, FNM_NOMATCH },
	{ "a/*b", "a/.b", FNM_PATHNAME|FNM_PERIOD, FNM_NOMATCH },
	{ "[.]a/b", ".a/b", FNM_PATHNAME|FNM_PERIOD, FNM_NOMATCH },
	{ "a/[.]b", "a/.b", FNM_PATHNAME|FNM_PERIOD, FNM_NOMATCH },
	{ "*/?", "a/b", FNM_PATHNAME|FNM_PERIOD, 0 },
	{ "?/*", "a/b", FNM_PATHNAME|FNM_PERIOD, 0 },
	{ ".*/?", ".a/b", FNM_PATHNAME|FNM_PERIOD, 0 },
	{ "*/.?", "a/.b", FNM_PATHNAME|FNM_PERIOD, 0 },
	{ "*/*", "a/.b", FNM_PATHNAME|FNM_PERIOD, FNM_NOMATCH },
	{ "*?*/*", "a/.b", FNM_PERIOD, 0 },
	{ "*[.]/b", "a./b", FNM_PATHNAME|FNM_PERIOD, 0 },
	{ "*[[:alpha:]]/*[[:alnum:]]", "a/b", FNM_PATHNAME, 0 },
	/* These three tests should result in error according to SUSv3.
	 * See XCU 2.13.1, XBD 9.3.5, & fnmatch() */
	{ "*[![:digit:]]*/[![:d-d]", "a/b", FNM_PATHNAME, -FNM_NOMATCH },
	{ "*[![:digit:]]*/[[:d-d]", "a/[", FNM_PATHNAME, -FNM_NOMATCH },
	{ "*[![:digit:]]*/[![:d-d]", "a/[", FNM_PATHNAME, -FNM_NOMATCH },
	{ "a?b", "a.b", FNM_PATHNAME|FNM_PERIOD, 0 },
	{ "a*b", "a.b", FNM_PATHNAME|FNM_PERIOD, 0 },
	{ "a[.]b", "a.b", FNM_PATHNAME|FNM_PERIOD, 0 },
};

void test_fnmatch(void) {
	int i;

	for (i = 0; i < sizeof(tests) / sizeof(*tests); i++) {
		int r, x;

		r = fnmatch(tests[i].pattern, tests[i].string, tests[i].flags);
		x = tests[i].expected;
		if (r != x && (r != FNM_NOMATCH || x != -FNM_NOMATCH)) {
			error("fail - fnmatch(\"%s\", \"%s\", ",
					tests[i].pattern, tests[i].string);
			printflags(fnmatch_flags, tests[i].flags);
			fprintf(stderr, ") => %d (expected %d)\n",
					r, tests[i].expected);
		}
	}
}
