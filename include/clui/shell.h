#ifndef _CLUI_SHELL_H
#define _CLUI_SHELL_H

#include <clui/clui.h>
#include <stdbool.h>

struct clui_shell_expr {
	unsigned int   nr;
	char         **words;
	char          *ln;
};

extern void
clui_shell_free_expr(const struct clui_shell_expr *expr) __nothrow __leaf;

extern int
clui_shell_read_expr(struct clui_shell_expr *expr) __clui_nonull(1);

extern void
clui_shell_shutdown(void) __nothrow __leaf;

extern void
clui_shell_redisplay(void) __nothrow __leaf;

extern void
clui_shell_init(const char *restrict name,
                const char *restrict prompt,
                bool                 enable_history);

extern void
clui_shell_fini(void);

#endif /* _CLUI_SHELL_H */
