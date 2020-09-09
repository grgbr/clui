#ifndef _LIBCLUI_H
#define _LIBCLUI_H

#include <clui/config.h>
#include <utils/cdefs.h>
#include <stdio.h>
#include <getopt.h>
#include <linux/taskstats.h>

#if defined(CONFIG_CLUI_ASSERT)

#include <utils/assert.h>

#define clui_assert(_expr) \
	uassert("clui", _expr)

#define __clui_nonull(_arg_index, ...)

#else  /* !defined(CONFIG_CLUI_ASSERT) */

#define clui_assert(_expr)

#define __clui_nonull(_arg_index, ...) \
	__nonull(_arg_index, ## __VA_ARGS__)

#endif /* defined(CONFIG_CLUI_ASSERT) */

struct clui_cmd;
struct clui_opt_set;

/******************************************************************************
 * Top-level parser handling
 ******************************************************************************/

struct clui_parser {
	const struct clui_opt_set *set;
	const struct clui_cmd     *cmd;
	char                       argv0[TS_COMM_LEN];
};

#define clui_assert_parser(_parser) \
	clui_assert(_parser); \
	clui_assert((_parser)->cmd); \
	clui_assert(*(_parser)->argv0); \
	clui_assert(!(_parser)->argv0[sizeof(parser->argv0) - 1])

extern void
clui_err(const struct clui_parser *restrict parser,
         const char               *restrict format,
         ...) __clui_nonull(1, 2) __printf(2, 3) __leaf;

extern int
clui_parse(struct clui_parser *parser,
           int                 argc,
           char * const       *argv,
           void               *ctx) __clui_nonull(1, 3);

extern int
clui_init(struct clui_parser        *restrict parser,
          const struct clui_opt_set *set,
          const struct clui_cmd     *cmd,
          int                        argc,
          char * const              *restrict argv) __clui_nonull(1, 3, 5)
                                                    __nothrow
                                                    __leaf;

/******************************************************************************
 * Parser option handling
 ******************************************************************************/

struct clui_opt;

typedef int (clui_parse_opt_fn)(const struct clui_opt    *opt,
                                const struct clui_parser *parser,
                                const char               *arg,
                                void                     *ctx);

struct clui_opt {
	int                short_char;
	const char        *long_name;
	int                has_arg;
	clui_parse_opt_fn *parse;
};

#define CLUI_OPT_NONE_ARG     (no_argument)
#define CLUI_OPT_REQUIRED_ARG (required_argument)
#define CLUI_OPT_OPTIONAL_ARG (optional_argument)

struct clui_opt_set;

typedef int (clui_check_opts_fn)(const struct clui_opt_set *set,
                                 const struct clui_parser  *parser,
                                 void                      *ctx);


typedef void (clui_help_opts_fn)(const struct clui_parser *parser,
                                 FILE                     *stdio);

struct clui_opt_set {
	unsigned int           nr;
	const struct clui_opt *opts;
	clui_check_opts_fn    *check;
	clui_help_opts_fn     *help;
};

#define clui_assert_opt_set(_set) \
	clui_assert(_set); \
	clui_assert((_set)->nr); \
	clui_assert((_set)->opts); \
	clui_assert((_set)->help)

static inline void __clui_nonull(1, 2)
clui_help_opts(const struct clui_parser *parser, FILE *stdio)
{
	clui_assert_parser(parser);
	clui_assert_opt_set(parser->set);
	clui_assert(stdio);

	parser->set->help(parser, stdio);
}

/******************************************************************************
 * Parser command handling
 ******************************************************************************/

typedef int (clui_parse_fn)(const struct clui_cmd *cmd,
                            struct clui_parser    *parser,
                            int                    argc,
                            char * const          *argv,
                            void                  *ctx);

typedef void (clui_help_fn)(const struct clui_cmd    *cmd,
                            const struct clui_parser *parser,
                            FILE                     *stdio);

struct clui_cmd {
	clui_parse_fn *parse;
	clui_help_fn  *help;
};

#define clui_assert_cmd(_cmd) \
	clui_assert(_cmd); \
	clui_assert((_cmd)->parse); \
	clui_assert((_cmd)->help)

static inline void __clui_nonull(1, 2, 3)
clui_help_cmd(const struct clui_cmd    *cmd,
              const struct clui_parser *parser,
              FILE                     *stdio)
{
	clui_assert_cmd(cmd);
	clui_assert_parser(parser);
	clui_assert(stdio);

	cmd->help(cmd, parser, stdio);
}

static inline int __clui_nonull(1, 2, 4)
clui_parse_cmd(const struct clui_cmd *cmd,
               struct clui_parser    *parser,
               int                    argc,
               char * const          *argv,
               void                  *ctx)
{
	clui_assert_cmd(cmd);
	clui_assert_parser(parser);
	clui_assert(argv);

	return cmd->parse(cmd, parser, argc, argv, ctx);
}

/******************************************************************************
 * Keyword parameter handling
 ******************************************************************************/

typedef int (clui_parse_kword_parm_fn)(const struct clui_cmd *cmd,
                                       struct clui_parser    *parser,
                                       const char            *argv,
                                       void                  *ctx);

#define CLUI_KWORD_PARM_MAX (32U)

struct clui_kword_parm {
	const char               *kword;
	clui_parse_kword_parm_fn *parse;
};

extern int
clui_parse_one_kword_parm(
	const struct clui_cmd                *cmd,
        struct clui_parser                   *parser,
        const struct clui_kword_parm * const  parms[],
        unsigned int                          nr,
        int                                   argc,
        char * const                          argv[],
	void                                 *ctx) __clui_nonull(1, 2, 3, 6);

extern int
clui_parse_all_kword_parms(
	const struct clui_cmd                *cmd,
        struct clui_parser                   *parser,
        const struct clui_kword_parm * const  parms[],
        unsigned int                          nr,
        int                                   argc,
        char * const                          argv[],
	void                                 *ctx) __clui_nonull(1, 2, 3, 6);

#endif /* _LIBCLUI_H */
