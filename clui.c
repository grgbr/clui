#include "clui/clui.h"
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>

/******************************************************************************
 * Helpers
 ******************************************************************************/

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

/******************************************************************************
 * Keyword parameter handling
 ******************************************************************************/

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
		clui_assert(parm->label);
		clui_assert(strnlen(parm->label, CLUI_LABEL_MAX) <
		            CLUI_LABEL_MAX);
		clui_assert(parm->parse);

		if (!strcmp(parm->label, argv[0]))
			/* Found it ! */
			break;
	}

	if (p == nr) {
		/* No matching keyword parm found. */
		clui_err(parser,
		         "unknown '%.*s' keyword.\n",
		         CLUI_LABEL_MAX - 1,
		         argv[0]);
		clui_help_cmd(cmd, parser, stderr);
		return -ENOENT;
	}

	/* Run the keyword parameter registered parser. */
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

/******************************************************************************
 * Switch parameter handling
 ******************************************************************************/

int __clui_nonull(1, 2, 3, 6)
clui_parse_one_switch_parm(const struct clui_cmd                 *cmd,
                           struct clui_parser                    *parser,
                           const struct clui_switch_parm * const  parms[],
                           unsigned int                           nr,
                           int                                    argc,
                           char * const                           argv[],
                           void                                  *ctx)
{
	clui_assert_cmd(cmd);
	clui_assert_parser(parser);
	clui_assert(parms);
	clui_assert(nr);
	clui_assert(argv);

	unsigned int                   p;
	const struct clui_switch_parm *parm;
	int                            ret;

	if ((argc < 1) || !argv[0] || !*argv[0]) {
		clui_err(parser, "missing keyword.\n");
		clui_help_cmd(cmd, parser, stderr);
		return -EINVAL;
	}

	/* Seach for a switch keyword matching the given argument. */
	for (p = 0; p < nr; p++) {
		parm = parms[p];

		clui_assert(parm);
		clui_assert(parm->label);
		clui_assert(strnlen(parm->label, CLUI_LABEL_MAX) <
		            CLUI_LABEL_MAX);
		clui_assert(parm->parse);

		if (!strcmp(parm->label, argv[0]))
			/* Found it ! */
			break;
	}

	if (p == nr) {
		/* No matching switch keyword found. */
		clui_err(parser,
		         "unknown '%.*s' keyword.\n",
		         CLUI_LABEL_MAX - 1,
		         argv[0]);
		clui_help_cmd(cmd, parser, stderr);
		return -ENOENT;
	}

	/* Run the switch registered parser. */
	ret = parm->parse(cmd, parser, ctx);
	if (ret)
		return ret;

	/* Tell the caller that we consummed 1 arguments. */
	return 1;
}

int __clui_nonull(1, 2, 3, 6)
clui_parse_all_switch_parms(const struct clui_cmd                 *cmd,
                            struct clui_parser                    *parser,
                            const struct clui_switch_parm * const  parms[],
                            unsigned int                           nr,
                            int                                    argc,
                            char * const                           argv[],
                            void                                  *ctx)
{
	do {
		int ret;

		ret = clui_parse_one_switch_parm(cmd,
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

/******************************************************************************
 * Parser option handling
 ******************************************************************************/

#define clui_assert_opt(_opt) \
	({ \
		clui_assert(_opt); \
		clui_assert(isalnum((_opt)->short_char)); \
		clui_assert((_opt)->long_name); \
		clui_assert(*(_opt)->long_name); \
		clui_assert((_opt)->has_arg >= no_argument); \
		clui_assert((_opt)->has_arg <= optional_argument); \
		clui_assert((_opt)->parse); \
	 })

int __clui_nonull(1, 2, 4)
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
	clui_help_opts(set, parser, stderr);

	return -EINVAL;
}

/******************************************************************************
 * Top-level parser handling
 ******************************************************************************/

int __clui_nonull(1, 2, 3, 5)
clui_parse(struct clui_parser        *parser,
           const struct clui_opt_set *set,
           const struct clui_cmd     *cmd,
           int                        argc,
           char                      *const *argv,
           void                      *ctx)
{
	clui_assert_parser(parser);
	clui_assert(set || cmd);
#if defined(CONFIG_CLUI_ASSERT)
	if (set)
		clui_assert_opt_set(set);
	if (cmd)
		clui_assert_cmd(cmd);
#endif /* defined(CONFIG_CLUI_ASSERT) */
	clui_assert(argc);
	clui_assert(argv);
	clui_assert(argv[0]);
	clui_assert(*argv[0]);

	int cnt = 1;

	if (set) {
		cnt = clui_parse_opts(set, parser, argc, argv, ctx);
		if (cnt < 0)
			return cnt;
	}

	if (!cmd)
		return 0;

	return clui_parse_cmd(cmd, parser, argc - cnt, &argv[cnt], ctx);
}

bool clui_color_on;

int __clui_nonull(1, 3) __nothrow __leaf
clui_init(struct clui_parser *restrict parser,
          int                 argc,
          char * const       *restrict argv)
{
	clui_assert(parser);
	clui_assert(argv);

	if (!argc || !argv[0] || !*argv[0])
		return -EINVAL;

	strncpy(parser->argv0, basename(argv[0]), sizeof(parser->argv0) - 1);
	parser->argv0[sizeof(parser->argv0) - 1] = '\0';

	if (isatty(STDOUT_FILENO))
		/*
		 * FIXME: in addition probe terminal color support using
		 * tigetnum("colors") from ncurses / termcap / terminfo ?
		 */
		clui_color_on = true;

	return 0;
}
