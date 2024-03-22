#ifndef _LIBCLUI_TABLE_H
#define _LIBCLUI_TABLE_H

#include <clui/clui.h>
#include <libsmartcols/libsmartcols.h>
#include <stdint.h>
#include <errno.h>

struct clui_table;

struct clui_column_desc {
	const char * label;
	double       whint;
	int          flags;
};

typedef int (clui_table_load_fn)(struct clui_table *,
                                 const struct clui_parser *,
                                 void *);

struct clui_table_desc {
	clui_table_load_fn *            load;
	unsigned int                    noheadings:1;
	unsigned int                    col_cnt;
	const struct clui_column_desc * columns;
};

#define clui_table_assert_desc(_desc) \
	clui_assert(_desc); \
	clui_assert((_desc)->load); \
	clui_assert((_desc)->col_cnt); \
	clui_assert((_desc)->columns)

struct clui_table {
	struct libscols_table *        scols;
	const struct clui_table_desc * desc;
};

#define clui_table_assert(_table) \
	clui_assert(_table); \
	clui_assert((_table)->scols); \
	clui_table_assert_desc((_table)->desc)

extern int
clui_table_line_set_uint(struct libscols_line * line,
                         unsigned int           column,
                         unsigned int           data);

extern int
clui_table_line_set_hex64(struct libscols_line * line,
                          unsigned int           column,
                          uint64_t               data);

static inline int
clui_table_line_set_str(struct libscols_line * line,
                        unsigned int           column,
                        const char *           data)
{
	clui_assert(line);

	int err;

	/*
	 * Request libsmartcols to duplicate string given as argument so that
	 * it may free(3) it once no more needed.
	 */
	err = scols_line_set_data(line, column, data);
	if (!err)
		return 0;

	clui_assert(err == -ENOMEM);

	return err;
}

static inline struct libscols_line *
clui_table_new_line(const struct clui_table * table,
                    struct libscols_line *    parent)
{
	clui_table_assert(table);

	struct libscols_line * line;

	line = scols_table_new_line(table->scols, parent);
	if (line)
		return line;

	clui_assert(errno == ENOMEM);

	return NULL;
}

extern void
clui_table_sort(const struct clui_table * table, unsigned int column);

static inline void
clui_table_clear(const struct clui_table * table)
{
	clui_table_assert(table);

	scols_table_remove_lines(table->scols);
}

extern int
clui_table_load(struct clui_table *        table,
                const struct clui_parser * parser,
                void *                     data);

extern int
clui_table_display(const struct clui_table *             table,
                   const struct clui_parser * __restrict parser);

extern int
clui_table_init(struct clui_table * table, const struct clui_table_desc * desc);

static inline void
clui_table_fini(struct clui_table * table)
{
	clui_table_assert(table);

	scols_unref_table(table->scols);
}

#endif /* _LIBCLUI_TABLE_H */
