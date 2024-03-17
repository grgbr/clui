#ifndef _LIBCLUI_SHELL_H
#define _LIBCLUI_SHELL_H

#include <clui/clui.h>
#include <stdbool.h>
#include <readline/readline.h>

static inline void
clui_shell_inhibit_completion_char(void)
{
	rl_completion_suppress_append = 1;
}

extern char **
clui_shell_build_static_matches(const char * const matches[],
                                size_t             nr) __clui_nonull(1);

typedef char ** (clui_shell_build_kword_match_fn)(const char * word,
						  size_t       len,
						  void *       data);

struct clui_shell_kword_parm {
	const struct clui_kword_parm    *clui;
	clui_shell_build_kword_match_fn *build;
};

extern char **
clui_shell_build_kword_matches(
	const struct clui_shell_kword_parm * const parms[],
	unsigned int                               nr,
	int                                        argc,
	const char * const                         argv[],
	void *                                     data)
	__clui_nonull(1, 4);

extern char **
clui_shell_build_switch_matches(const struct clui_switch_parm * const parms[],
                                unsigned int                          nr,
                                int                                   argc,
                                const char * const                    argv[])
	__clui_nonull(1, 4);

struct clui_shell_expr {
	unsigned int  nr;
	char **       words;
	char *        ln;
};

extern void
clui_shell_free_expr(const struct clui_shell_expr * expr) __nothrow __leaf;

extern int
clui_shell_read_expr(struct clui_shell_expr * expr) __clui_nonull(1);

extern void
clui_shell_shutdown(void) __nothrow __leaf;

extern void
clui_shell_redisplay(void) __nothrow __leaf;

extern void
clui_shell_init(const struct clui_cmd * restrict cmd,
                struct clui_parser * restrict    parser,
                const char * restrict            name,
                const char * restrict            prompt,
                void * restrict                  ctx,
                bool                             enable_history);

extern void
clui_shell_fini(struct clui_parser * restrict parser);

#endif /* _LIBCLUI_SHELL_H */
