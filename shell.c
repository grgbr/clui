#include <clui/clui.h>
#include <clui/shell.h>
#include <utils/string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <readline/readline.h>
#include <readline/history.h>

struct clui_shell {
	const char   *prompt;
	bool          hist;
	sig_atomic_t  shutdown;
};

static struct clui_shell clui_the_shell;

static int
clui_shell_read_line(char **line)
{
	clui_assert(!clui_the_shell.prompt || *clui_the_shell.prompt);
	clui_assert(line);

	char *ln;

	ln = readline(clui_the_shell.prompt);
	if (!ln) {
		/* End of stream required using ^D. */
		putchar('\n');
		return -ESHUTDOWN;
	}

	if (clui_the_shell.shutdown) {
		free(ln);
		return -ESHUTDOWN;
	}

	if (!*ln) {
		free(ln);
		return -ENODATA;
	}

	*line = ln;

	return 0;
}

static void
clui_shell_hist_expr(const struct clui_shell_expr *expr, size_t max_size)
{
	clui_assert(expr);
	clui_assert(expr->nr);
	clui_assert(expr->words);
	clui_assert(expr->ln);
	clui_assert(max_size > 1);

	unsigned int  w;
	char         *ln;
	char         *ptr;

	ln = malloc(max_size);
	if (!ln)
		return;

	ptr = stpcpy(ln, expr->words[0]);
	clui_assert((size_t)(ptr - ln) < max_size);

	for (w = 1; w < expr->nr; w++) {
		*ptr++ = ' ';
		clui_assert((size_t)(ptr - ln) < max_size);

		ptr = stpcpy(ptr, expr->words[w]);
		clui_assert((size_t)(ptr - ln) < max_size);
	}

	add_history(ln);

	free(ln);
}

void __nothrow __leaf
clui_shell_free_expr(const struct clui_shell_expr *expr)
{
	clui_assert(expr);
	clui_assert(expr->nr);
	clui_assert(expr->words);
	clui_assert(expr->ln);

	free(expr->words);
	free(expr->ln);
}

int __clui_nonull(1)
clui_shell_read_expr(struct clui_shell_expr *expr)
{
	clui_assert(expr);

	char         *ln;
	int           ret;
	ssize_t       len;
	unsigned int  nr;
	unsigned int  pos;

	ret = clui_shell_read_line(&ln);
	if ((ret == -ESHUTDOWN) || (ret == -ENODATA))
		return ret;

	len = ustr_parse(ln, LINE_MAX - 1);
	if (len < 0) {
		ret = len;
		goto free_ln;
	}

	nr = 8;
	expr->words = malloc(nr * sizeof(expr->words[0]));
	if (!expr->words) {
		ret = -ENOMEM;
		goto free_ln;
	}
	expr->nr = 0;

	pos = 0;
	do {
		unsigned int cnt;

		pos += ustr_skip_space(&ln[pos], len - pos);
		clui_assert(pos <= (size_t)len);
		if (!(len - pos))
			/* End of input line. */
			break;

		cnt = ustr_skip_notspace(&ln[pos], len - pos);
		clui_assert(cnt);

		ln[pos + cnt] = '\0';

		clui_assert(expr->nr <= nr);
		if (expr->nr == nr) {
			char **words;

			nr *= 2;
			words = realloc(expr->words,
			                (nr * sizeof(expr->words[0])));
			if (!words) {
				ret = -ENOMEM;
				goto free_ln;
			}

			expr->words = words;
		}

		expr->words[expr->nr++] = &ln[pos];

		pos += cnt + 1;
	} while (pos < (size_t)len);

	if (!expr->nr) {
		ret = -ENODATA;
		goto free_ln;
	}

	expr->ln = ln;

	if (clui_the_shell.hist)
		clui_shell_hist_expr(expr, len + 1);

	return 0;

free_ln:
	free(ln);

	return ret;
}

void __nothrow __leaf
clui_shell_shutdown(void)
{
	clui_the_shell.shutdown = 1;
}

static int
clui_shell_handle_readline_events(void)
{
	if (clui_the_shell.shutdown)
		/*
		 * Cause a blocking readline() to return immediately instead of
		 * waiting for the next end-of-line input character.
		 */
		rl_done = 1;

	return 0;
}

void __nothrow __leaf
clui_shell_init(const char *prompt, bool enable_history)
{
	clui_assert(!prompt || *prompt);

	clui_the_shell.prompt = prompt;
	clui_the_shell.hist = enable_history;
	clui_the_shell.shutdown = 0;

	rl_event_hook = clui_shell_handle_readline_events;
}
