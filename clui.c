#include <clui/clui.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

void __clui_nonull(1, 2) __printf(2, 3) __leaf
clui_err(const struct clui_parser *restrict parser,
         const char               *restrict format,
         ...)
{
	clui_assert_parser(parser);

	va_list args;

	fprintf(stderr, "%s: ", parser->argv0);

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);

	putc('\n', stderr);
}

int __clui_nonull(1, 2, 3, 6)
clui_parse_one_kword_parm(const struct clui_cmd                *cmd,
                          struct clui_parser                   *parser,
                          const struct clui_kword_parm * const  parms[],
                          unsigned int                          nr,
                          int                                   argc,
                          char * const                          argv[],
                          void                                 *ctx)
{
	clui_assert_cmd(cmd);
	clui_assert_parser(parser);
	clui_assert(parms);
	clui_assert(nr);
	clui_assert(argv);

	unsigned int                  p;
	const struct clui_kword_parm *parm;
	int                           ret;

	if ((argc < 2) || !argv[0] || !*argv[0] || !argv[1] || !*argv[1]) {
		clui_err(parser, "missing keyword parameter(s).\n");
		clui_help_cmd(cmd, parser, stderr);
		return -EINVAL;
	}

	/* Seach for a keyword parameter matching the first given argument. */
	for (p = 0; p < nr; p++) {
		parm = parms[p];

		clui_assert(parm);
		clui_assert(parm->kword);
		clui_assert(strnlen(parm->kword, CLUI_KWORD_PARM_MAX) <
		            CLUI_KWORD_PARM_MAX);
		clui_assert(parm->parse);

		if (!strcmp(parm->kword, argv[0]))
			/* Found it ! */
			break;
	}

	if (p == nr) {
		/* No matching keyword parm found. */
		clui_err(parser,
		         "unknown '%.*s' keyword.\n",
		         CLUI_KWORD_PARM_MAX - 1,
		         argv[0]);
		clui_help_cmd(cmd, parser, stderr);
		return -ENOENT;
	}

	/* Run the keyword parameter register parser. */
	ret = parm->parse(cmd, parser, argv[1], ctx);
	if (ret)
		return ret;

	/* Tell the caller that we consummed 2 arguments. */
	return 2;
}

int __clui_nonull(1, 2, 3, 6)
clui_parse_all_kword_parms(const struct clui_cmd                *cmd,
                           struct clui_parser                   *parser,
                           const struct clui_kword_parm * const  parms[],
                           unsigned int                          nr,
                           int                                   argc,
                           char * const                          argv[],
                           void                                 *ctx)
{
	do {
		int ret;

		ret = clui_parse_one_kword_parm(cmd,
		                                parser,
		                                parms,
		                                nr,
		                                argc,
		                                argv,
		                                ctx);
		clui_assert(ret <= argc);
		if (ret < 0)
			return ret;

		argc -= ret;
		argv = &argv[ret];
	} while (argc);

	return 0;
}

int __clui_nonull(1, 3)
clui_parse(struct clui_parser *parser, int argc, char * const *argv, void *ctx)
{
	clui_assert_parser(parser);
	clui_assert(argc);
	clui_assert(argv);
	clui_assert(argv[0]);
	clui_assert(*argv[0]);

	return clui_parse_cmd(parser->cmd, parser, argc - 1, &argv[1], ctx);
}

int __clui_nonull(1, 2, 4) __nothrow __leaf
clui_init(struct clui_parser    *restrict parser,
          const struct clui_cmd *cmd,
          int                    argc,
          char * const          *restrict argv)
{
	clui_assert(parser);
	clui_assert_cmd(cmd);
	clui_assert(argv);

	if (!argc || !argv[0] || !*argv[0])
		return -EINVAL;

	parser->cmd = cmd;

	strncpy(parser->argv0, basename(argv[0]), sizeof(parser->argv0) - 1);
	parser->argv0[sizeof(parser->argv0) - 1] = '\0';

	return 0;
}
