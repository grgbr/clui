#ifndef _LIBCLUI_TABLE_H
#define _LIBCLUI_TABLE_H

#include <clui/clui.h>
#include <libsmartcols/libsmartcols.h>

struct clui_table;

struct clui_column_desc {
	const char * label;
	double       whint;
	int          flags;
};

typedef int (clui_table_update_fn)(struct clui_table *, void *);

struct clui_table_desc {
	clui_table_update_fn *          update;
	unsigned int                    noheadings:1;
	unsigned int                    col_cnt;
	const struct clui_column_desc * columns;
};

#define clui_table_assert_desc(_desc) \
	clui_assert(_desc); \
	clui_assert((_desc)->update); \
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

static inline int
clui_table_line_set_str(struct libscols_line * line,
                        unsigned int           column,
                        const char *           data)
{
	clui_assert(line);

	/*
	 * Request libsmartcols to duplicate string given as argument so that
	 * it may free(3) it once no more needed.
	 */
	return scols_line_set_data(line, column, data);
}

static inline struct libscols_line *
clui_table_new_line(const struct clui_table * table,
                    struct libscols_line *    parent)
{
	clui_table_assert(table);

	return scols_table_new_line(table->scols, parent);
}

extern void
clui_table_sort(const struct clui_table * table, unsigned int column);

static inline void
clui_table_clear(const struct clui_table * table)
{
	clui_table_assert(table);

	scols_table_remove_lines(table->scols);
}

static inline int
clui_table_update(struct clui_table * table, void * data)
{
	clui_table_assert(table);

	return table->desc->update(table, data);
}

static inline int
clui_table_display(const struct clui_table * table)
{
	clui_table_assert(table);

	return scols_print_table(table->scols);
}

extern int
clui_table_init(struct clui_table * table, const struct clui_table_desc * desc);

static inline void
clui_table_fini(struct clui_table * table)
{
	clui_table_assert(table);

	scols_unref_table(table->scols);
}

#endif /* _LIBCLUI_TABLE_H */
