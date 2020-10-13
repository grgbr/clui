#include <clui/clui.h>
#include <clui/shell.h>
#include <utils/string.h>
#include <utils/path.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <readline/readline.h>
#include <readline/history.h>

struct clui_shell {
	const char            *prompt;
	bool                   hist;
	char                  *hist_path;
	volatile sig_atomic_t  redisplay;
	volatile sig_atomic_t  shutdown;
};

static struct clui_shell clui_the_shell;

static int
clui_shell_read_line(char **line)
{
	clui_assert(!clui_the_shell.prompt || *clui_the_shell.prompt);
	clui_assert(line);

	char *ln;

	/* Reset redisplay event handling logic. */
	clui_the_shell.redisplay = 0;

	ln = readline(clui_the_shell.prompt);
	if (!ln) {
		/* End of stream required using ^D. */
		rl_crlf();
		return -ESHUTDOWN;
	}

	if (clui_the_shell.shutdown) {
		/* We were explicitly requested to shutdown. */
		free(ln);
		return -ESHUTDOWN;
	}

	if (clui_the_shell.redisplay || !*ln) {
		/*
		 * When clui_the_shell.redisplay is true, a redisplay event
		 * happened during the course of readline() and it has not been
		 * processed entirely, i.e. we were interrupted in the middle of
		 * an incremental searching and / or numeric argument probing
		 * operation.
		 * Complete the event by freeing readline's buffered input.
		 * See clui_shell_handle_readline_events();
		 */
		
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

void __nothrow __leaf
clui_shell_redisplay(void)
{
	clui_the_shell.redisplay = 1;
}

static int
clui_shell_handle_readline_events(void)
{
	if (clui_the_shell.redisplay) {
		/* Move cursor to next line. */
		rl_crlf();
		/* Tell readline we have moved onto a new empty line. */
		rl_on_new_line();
		/* Wipe buffered line content out. */
		rl_replace_line("", 0);

		if (RL_ISSTATE(RL_STATE_ISEARCH | RL_STATE_NUMERICARG)) {
			/*
			 * We are in the middle of an incremental searching or
			 * numeric argument probing operation.
			 * Restore our own prompt since both operations
			 * temporarily modify current prompt and that we have
			 * just interrupted them.
			 */
			rl_restore_prompt();

			/*
			 * Cause a blocking readline() to return immediately
			 * instead of waiting for the next end-of-line input
			 * character.
			 * This gives readline() caller a chance to purge
			 * buffered input as soon as possible to prevent from
			 * mixing it with post interruption input characters.
			 * The reason why we cannot free these buffered
			 * characters here is that readline provides no public
			 * API (I know of) to purge characters buffered by its
			 * incremental searching logic. As a consequence we need
			 * to wait for readline() to return before being able to
			 * release them.
			 * This is the reason we leave clui_the_shell.redisplay
			 * untouched here...
			 */
			rl_done = 1;
		}
		else {
			/* Tell readline it should update screen. */
			rl_redisplay();

			/* Mark redisplay event as completed. */
			clui_the_shell.redisplay = 0;
		}
	}
	else if (clui_the_shell.shutdown)
		/* Request readline() to return immediately. */
		rl_done = 1;

	return 0;
}

static char *
clui_shell_hist_path(const char *name)
{
	const char *base_dir;
	char       *conf_dir;
	char       *hist_path;
	int         err;

	err = upath_validate_file_name(name);
	if (err < 0) {
		errno = -err;
		return NULL;
	}

	base_dir = secure_getenv("XDG_CONFIG_HOME");
	if (!base_dir) {
		base_dir = secure_getenv("HOME");
		if (!base_dir)
			return NULL;

		err = asprintf(&conf_dir, "%s/.config/%s", base_dir, name);
	}
	else
		err = asprintf(&conf_dir, "%s/%s", base_dir, name);

	if (err < 0)
		return NULL;

	if (mkdir(conf_dir, S_IRUSR | S_IWUSR)) {
		clui_assert(errno != EFAULT);
		if (errno != EEXIST)
			goto free;
	}

	err = asprintf(&hist_path, "%s/history", conf_dir);
	if (err < 0)
		goto free;

	free(conf_dir);

	return hist_path;

free:
	err = errno;
	free(conf_dir);

	errno = err;
	return NULL;
}

void
clui_shell_init(const char *restrict name,
                const char *restrict prompt,
                bool                 enable_history)
{
	clui_assert(!name || *name);
	clui_assert(!prompt || *prompt);

	clui_the_shell.prompt = prompt;
	clui_the_shell.hist = enable_history;
	clui_the_shell.hist_path = NULL;
	clui_the_shell.shutdown = 0;

	if (enable_history) {
		if (name) {
			clui_the_shell.hist_path = clui_shell_hist_path(name);
			if (clui_the_shell.hist_path)
				read_history(clui_the_shell.hist_path);
		}
	}

	if (name)
		rl_readline_name = name;

	rl_event_hook = clui_shell_handle_readline_events;
}

static void
clui_shell_save_hist(void)
{
	if (clui_the_shell.hist_path) {
		clui_assert(clui_the_shell.hist);

		write_history(clui_the_shell.hist_path);

		free(clui_the_shell.hist_path);
	}
}

void
clui_shell_fini(void)
{
	clui_shell_save_hist();
}
