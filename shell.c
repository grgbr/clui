#include "clui/shell.h"
#include <utils/string.h>
#include <utils/path.h>
#include <utils/bitmap.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <readline/readline.h>
#include <readline/history.h>

struct clui_shell {
	size_t                   match_len;
	unsigned int             match_indx;
	unsigned int             match_nr;
	const char * const *     matches;
	clui_shell_complete_fn * complete;
	void *                   data;
	const char *             prompt;
	bool                     hist;
	char *                   hist_path;
	volatile sig_atomic_t    redisplay;
	volatile sig_atomic_t    shutdown;
};

static struct clui_shell clui_the_shell;

static char * __clui_nonull(1) __nothrow
clui_shell_generate_static_match(const char * restrict word, int state __unused)
{
	clui_assert(word);
	clui_assert(!(!!state ^ !!clui_the_shell.match_indx));
	clui_assert(clui_the_shell.match_nr);
	clui_assert(clui_the_shell.matches);

	while (clui_the_shell.match_indx < clui_the_shell.match_nr) {
		const char * match =
			clui_the_shell.matches[clui_the_shell.match_indx];

		if (!match)
			break;

		clui_the_shell.match_indx++;

		if (!strncmp(word, match, clui_the_shell.match_len))
			return strdup(match);
	}

	return NULL;
}

char ** __clui_nonull(1, 3)
clui_shell_build_static_matches(const char *       word,
                                size_t             len,
                                const char * const matches[],
                                size_t             nr)
{
	clui_assert(word);
	clui_assert(matches);
	clui_assert(nr);

	if (len >= (LINE_MAX - 1))
		return NULL;

	clui_assert(strnlen(word, LINE_MAX) == len);

	clui_the_shell.match_len = len;
	clui_the_shell.match_indx = 0;
	clui_the_shell.matches = matches;
	clui_the_shell.match_nr = nr;

	return rl_completion_matches(word, clui_shell_generate_static_match);
}

static int __clui_nonull(1, 3) __nothrow __clui_pure
clui_shell_find_kword_parm(
	const struct clui_shell_kword_parm * const restrict parms[],
	unsigned int                                        nr,
	const char *                                        arg)
{
	clui_assert(parms);
	clui_assert(nr);
	clui_assert(arg);

	if (*arg) {
		unsigned int p;

		for (p = 0; p < nr; p++) {
			clui_assert(parms[p]);

			const struct clui_kword_parm * clui = parms[p]->clui;

			clui_assert(clui);
			clui_assert(clui->label);
			clui_assert(strnlen(clui->label, CLUI_LABEL_MAX) <
				    CLUI_LABEL_MAX);
			clui_assert(clui->parse);

			if (!strcmp(clui->label, arg))
				return p;
		}
	}

	return -ENOENT;
}

char ** __clui_nonull(1, 3, 6)
clui_shell_build_kword_matches(
	const char *                               word,
	size_t                                     len,
	const struct clui_shell_kword_parm * const parms[],
	unsigned int                               nr,
	int                                        argc,
	const char * const                         argv[],
	void *                                     data)
{
	clui_assert(word);
	clui_assert(parms);
	clui_assert(nr);
	clui_assert(argv);

	struct fbmp bmp;
	int         p = -ENOENT;
	char **     matches = NULL;

	if (fbmp_init_set(&bmp, nr))
		return NULL;

	if (argc > 0) {
		int a;

		for (a = 0; a < (argc - 1); a += 2) {
			p = clui_shell_find_kword_parm(parms, nr, argv[a]);
			if (p >= 0)
				fbmp_clear(&bmp, p);
		}

		p = clui_shell_find_kword_parm(parms, nr, argv[argc - 1]);
	}

	clui_assert(p < (int)nr);

	if (!(argc % 2)) {
		const char **    samples;
		struct fbmp_iter iter;
		unsigned int     m = 0;

		samples = malloc(nr * sizeof(samples[0]));
		if (!samples)
			goto fini;

		if (p >= 0)
			fbmp_clear(&bmp, p);

		fbmp_foreach_bit(&iter, &bmp, p) {
			clui_assert(parms[p]);
			clui_assert(parms[p]->clui);
			clui_assert(parms[p]->clui->label);

			samples[m++] = parms[p]->clui->label;
		}

		if (m)
			matches = clui_shell_build_static_matches(word,
			                                          len,
			                                          samples,
			                                          m);

		free(samples);
	}
	else if ((p >= 0) && fbmp_test(&bmp, p)) {
		clui_assert(parms[p]);
		clui_assert(parms[p]->clui);
		clui_assert(parms[p]->clui->label);

		if (parms[p]->build)
			matches = parms[p]->build(word, len, data);
		else
			matches = NULL;
	}

fini:
	fbmp_fini(&bmp);

	return matches;
}

static int __clui_nonull(1, 3) __nothrow __clui_pure
clui_shell_find_switch_parm(
	const struct clui_switch_parm * const restrict parms[],
	unsigned int                                   nr,
	const char *                                   arg)
{
	clui_assert(parms);
	clui_assert(nr);
	clui_assert(arg);

	if (*arg) {
		unsigned int p;

		for (p = 0; p < nr; p++) {
			const struct clui_switch_parm * parm = parms[p];

			clui_assert(parm);
			clui_assert(parm->label);
			clui_assert(strnlen(parm->label, CLUI_LABEL_MAX) <
			            CLUI_LABEL_MAX);
			clui_assert(parm->parse);

			if (!strcmp(parm->label, arg))
				/* Found it ! */
				return p;
		}
	}

	return -ENOENT;
}

char ** __clui_nonull(1, 3, 6)
clui_shell_build_switch_matches(const char *                          word,
                                size_t                                len,
                                const struct clui_switch_parm * const parms[],
                                unsigned int                          nr,
                                int                                   argc,
                                const char * const                    argv[])
{
	clui_assert(word);
	clui_assert(parms);
	clui_assert(nr);
	clui_assert(argv);

	struct fbmp      bmp;
	const char **    samples;
	int              p = -ENOENT;
	struct fbmp_iter iter;
	unsigned int     m = 0;
	char **          matches = NULL;

	if (fbmp_init_set(&bmp, nr))
		return NULL;

	samples = malloc(nr * sizeof(samples[0]));
	if (!samples)
		goto fini;

	if (argc > 0) {
		int a;

		for (a = 0; a < argc; a ++) {
			p = clui_shell_find_switch_parm(parms, nr, argv[a]);
			if (p >= 0)
				fbmp_clear(&bmp, p);
		}
	}

	fbmp_foreach_bit(&iter, &bmp, p)
		samples[m++] = parms[p]->label;

	if (m)
		matches = clui_shell_build_static_matches(word,
		                                          len,
		                                          samples,
		                                          m);

	free(samples);

fini:
	fbmp_fini(&bmp);

	return matches;
}

static int
clui_shell_read_line(char ** line)
{
	clui_assert(!clui_the_shell.prompt || *clui_the_shell.prompt);
	clui_assert(line);

	char * ln;

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
clui_shell_hist_expr(const struct clui_shell_expr * expr, size_t max_size)
{
	clui_assert(expr);
	clui_assert(expr->nr);
	clui_assert(expr->words);
	clui_assert(expr->ln);
	clui_assert(max_size > 1);

	unsigned int   w;
	char         * ln;
	char         * ptr;

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

static int
clui_shell_break_expr(char *** restrict words,
                      char * restrict   line,
                      size_t            len)
{
	clui_assert(words);
	clui_assert(line);
	clui_assert(len);

	char **      toks;
	unsigned int nr;
	unsigned int pos;
	unsigned int cnt;
	int          ret;

	nr = 8;
	toks = malloc(nr * sizeof(toks[0]));
	if (!toks)
		return -errno;

	pos = 0;
	cnt = 0;
	do {
		unsigned int wlen;

		pos += ustr_skip_space(&line[pos], len - pos);
		clui_assert(pos <= len);
		if (!(len - pos))
			/* End of input line. */
			break;

		wlen = ustr_skip_notspace(&line[pos], len - pos);
		clui_assert(wlen);

		line[pos + wlen] = '\0';

		clui_assert(cnt <= nr);
		if (cnt == nr) {
			char **tmp;

			nr *= 2;
			tmp = realloc(toks, (nr * sizeof(toks[0])));
			if (!tmp) {
				ret = -errno;
				goto free;
			}

			toks = tmp;
		}

		toks[cnt++] = &line[pos];

		pos += wlen + 1;
	} while (pos < len);

	if (!cnt) {
		ret = 0;
		goto free;
	}

	*words = toks;

	return cnt;

free:
	free(toks);

	return ret;
}

int __clui_nonull(1)
clui_shell_read_expr(struct clui_shell_expr * expr)
{
	clui_assert(expr);

	char * ln;
	int    ret;
	int    len;

	ret = clui_shell_read_line(&ln);
	if ((ret == -ESHUTDOWN) || (ret == -ENODATA))
		return ret;

	len = ustr_parse(ln, LINE_MAX - 1);
	clui_assert(len);
	if (len < 0) {
		ret = len;
		goto free;
	}

	ret = clui_shell_break_expr(&expr->words, ln, len);
	if (ret <= 0) {
		ret = !ret ? -ENODATA : ret;
		goto free;
	}

	expr->nr = ret;
	expr->ln = ln;

	if (clui_the_shell.hist)
		clui_shell_hist_expr(expr, len + 1);

	return 0;

free:
	free(ln);

	return ret;
}

void __nothrow __leaf
clui_shell_free_expr(const struct clui_shell_expr * expr)
{
	clui_assert(expr);
	clui_assert(expr->nr);
	clui_assert(expr->words);
	clui_assert(expr->ln);

	free(expr->words);
	free(expr->ln);
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
clui_shell_hist_path(const char * name)
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

	if (mkdir(conf_dir, S_IRUSR | S_IWUSR | S_IXUSR)) {
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

static char ** __clui_nonull(1)
clui_shell_complete(const char * word, int start, int end)
{
	clui_assert(word);
	clui_assert(start >= 0);
	clui_assert(end >= 0);
	clui_assert(start <= end);
	clui_assert(end <= rl_end);
	clui_assert(strlen(word) == (size_t)(end - start));

	if (start) {
		char ** words;
		char *  ln;
		int     ret;
		char ** matches;

		if (end >= LINE_MAX)
			return NULL;

		ln = malloc(end + 1);
		if (!ln)
			return NULL;

		memcpy(ln, rl_line_buffer, end);
		ln[end] = '\0';

		ret = clui_shell_break_expr(&words, ln, start);
		if (ret > 0) {
			matches = clui_the_shell.complete(word,
			                                  end - start,
			                                  ret,
			                                  (const char * const *)
			                                  words,
			                                  clui_the_shell.data);
			free(words);
		}
		else if (!ret)
			matches = clui_the_shell.complete(word,
			                                  end - start,
			                                  0,
			                                  NULL,
			                                  clui_the_shell.data);
		else
			matches = NULL;

		free(ln);

		return matches;
	}

	return clui_the_shell.complete(word,
	                               end - start,
	                               0,
	                               NULL,
	                               clui_the_shell.data);
}

void
clui_shell_init(const char * restrict    name,
                const char * restrict    prompt,
                clui_shell_complete_fn * complete,
                void * restrict          data,
                bool                     enable_history)
{
	clui_assert(!name || *name);
	clui_assert(!prompt || *prompt);

	clui_the_shell.complete = complete;
	clui_the_shell.data = data;
	clui_the_shell.prompt = prompt;
	clui_the_shell.hist = enable_history;
	clui_the_shell.hist_path = NULL;
	clui_the_shell.shutdown = 0;

	if (complete) {
		rl_attempted_completion_function = clui_shell_complete;
		rl_inhibit_completion = 0;
	}
	else
		rl_inhibit_completion = 1;

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
