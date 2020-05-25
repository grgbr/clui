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
