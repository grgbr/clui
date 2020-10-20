#ifndef _CLUI_SHELL_H
#define _CLUI_SHELL_H

#include <clui/clui.h>
#include <stdbool.h>
#include <readline/readline.h>

extern char **
clui_shell_build_static_matches(const char *       word,
                                size_t             len,
                                const char * const matches[],
                                size_t             nr) __clui_nonull(1, 3);

typedef char ** (clui_shell_build_kword_match_fn)(const char * word,
						  size_t       len,
						  void *       data);

struct clui_shell_kword_parm {
	const struct clui_kword_parm    *clui;
	clui_shell_build_kword_match_fn *build;
};

extern char **
clui_shell_build_kword_matches(
	const char *                               word,
	size_t                                     len,
	const struct clui_shell_kword_parm * const parms[],
	unsigned int                               nr,
	int                                        argc,
	const char * const                         argv[],
	void *                                     data);
	__clui_nonull(1, 3, 6);

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

typedef char ** (clui_shell_complete_fn)(const char *       word,
					 size_t             match_len,
                                         int                argc,
                                         const char * const argv[],
                                         void *             data);

extern void
clui_shell_init(const char * restrict    name,
                const char * restrict    prompt,
                clui_shell_complete_fn * complete,
                void * restrict          data,
                bool                     enable_history);

extern void
clui_shell_fini(void);

#endif /* _CLUI_SHELL_H */
