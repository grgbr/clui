#include <clui/clui.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
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
		clui_err(parser, "missing keyword and/or parameter.\n");
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

#define clui_assert_opt(_opt) \
	clui_assert(_opt); \
	clui_assert(isalnum((_opt)->short_char)); \
	clui_assert((_opt)->long_name); \
	clui_assert(*(_opt)->long_name); \
	clui_assert((_opt)->has_arg >= no_argument); \
	clui_assert((_opt)->has_arg <= optional_argument); \
	clui_assert((_opt)->parse)

static int __clui_nonull(1, 2, 4)
clui_parse_opts(const struct clui_opt_set *set,
                struct clui_parser        *parser,
                int                        argc,
                char * const              *argv,
                void                      *ctx)
{
	clui_assert_opt_set(set);
	clui_assert_parser(parser);

	static const struct option  end = { 0, };
	unsigned int                o;
	char                        short_opts[(set->nr * 3) + 2];
	struct option               long_opts[set->nr + 1];
	char                       *s = short_opts;
	const struct clui_opt      *opt;
	int                         ret;

	*s++ = ':';
	for (o = 0; o < set->nr; o++) {
		struct option *l = &long_opts[o];

		opt = &set->opts[o];
		clui_assert_opt(opt);

		l->name = opt->long_name;
		l->has_arg = opt->has_arg;
		l->flag = NULL;
		l->val = opt->short_char;

		*s++ = opt->short_char;
		if (opt->has_arg == no_argument)
			continue;

		*s++ = ':';
		if (opt->has_arg == optional_argument)
			*s++ = ':';
	}

	*s = '\0';
	long_opts[o] = end;

	while (true) {
		ret = getopt_long(argc, argv, short_opts, long_opts, NULL);
		if (ret < 0)
			/* End of command line option parsing. */
			break;

		if (ret == '?') {
			clui_err(parser, "invalid option '%c'.\n", optopt);
			goto err;
		}
		else if (ret == ':') {
			clui_err(parser,
			         "option '%c' requires an argument.\n",
			         optopt);
			goto err;
		}

		for (o = 0; o < set->nr; o++) {
			opt = &set->opts[o];
			if (ret == opt->short_char)
				break;
		}

		clui_assert(o < set->nr);

		ret = opt->parse(opt,
		                 parser,
		                 (opt->has_arg != no_argument) ? optarg : NULL,
		                 ctx);
		if (ret)
			return ret;
	}

	if (set->check) {
		ret = set->check(set, parser, ctx);
		if (ret)
			return ret;
	}

	return optind;

err:
	clui_help_opts(parser, stderr);

	return -EINVAL;
}

int __clui_nonull(1, 3)
clui_parse(struct clui_parser *parser, int argc, char * const *argv, void *ctx)
{
	clui_assert_parser(parser);
	clui_assert(argc);
	clui_assert(argv);
	clui_assert(argv[0]);
	clui_assert(*argv[0]);

	int cnt = 1;

	if (parser->set) {
		cnt = clui_parse_opts(parser->set, parser, argc, argv, ctx);
		if (cnt < 0)
			return cnt;
	}

	return clui_parse_cmd(parser->cmd, parser, argc - cnt, &argv[cnt], ctx);
}

int __clui_nonull(1, 3, 5) __nothrow __leaf
clui_init(struct clui_parser        *restrict parser,
          const struct clui_opt_set *set,
          const struct clui_cmd     *cmd,
          int                        argc,
          char * const              *restrict argv)
{
	clui_assert(parser);
	clui_assert_cmd(cmd);
	clui_assert(argv);
#if defined(CONFIG_CLUI_ASSERT)
	if (set)
		clui_assert_opt_set(set);
#endif /* defined(CONFIG_CLUI_ASSERT) */

	if (!argc || !argv[0] || !*argv[0])
		return -EINVAL;

	parser->set = set;
	parser->cmd = cmd;

	strncpy(parser->argv0, basename(argv[0]), sizeof(parser->argv0) - 1);
	parser->argv0[sizeof(parser->argv0) - 1] = '\0';

	return 0;
}
