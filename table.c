#include "clui/table.h"
#include <errno.h>

int
clui_table_line_set_uint(struct libscols_line * line,
                         unsigned int           column,
                         unsigned int           data)
{
	clui_assert(line);

	char * str;
	int    err __unused;

	if (asprintf(&str, "%u", data) < 0)
		return -errno;

	/*
	 * Give ownership of str to libsmartcols so that it may free(3) it once
	 * no more needed.
	 * scols_line_refer_data() always returns 0 unless given line argument
	 * is NULL.
	 */
	err = scols_line_refer_data(line, column, str);
	clui_assert(!err);

	return 0;
}

void
clui_table_sort(const struct clui_table * table, unsigned int column)
{
	clui_table_assert(table);
	clui_assert(column < table->desc->col_cnt);

	struct libscols_column * col;
	int                      err;

	col = scols_table_get_column(table->scols, column);
	clui_assert(col);

	err = scols_column_set_cmpfunc(col, scols_cmpstr_cells, NULL);
	clui_assert(!err);

	err = scols_sort_table(table->scols, col);
	clui_assert(!err);
}

int
clui_table_init(struct clui_table * table, const struct clui_table_desc * desc)
{
	clui_assert(table);
	clui_table_assert_desc(desc);

	struct libscols_table * tbl;
	unsigned int            c;

	tbl = scols_new_table();
	if (!tbl)
		return -errno;

	scols_table_enable_colors(tbl, (int)clui_has_colors());
	scols_table_enable_noheadings(tbl, (int)desc->noheadings);

	for (c = 0; c < desc->col_cnt; c++) {
		const struct clui_column_desc * cdesc = &desc->columns[c];
		struct libscols_column *        col;

		clui_assert(cdesc->label);
		clui_assert(cdesc->label[0]);

		col = scols_table_new_column(tbl,
		                             cdesc->label,
		                             cdesc->whint,
		                             cdesc->flags);
		if (!col)
			goto unref;

		/* Highlight column header title. */
		scols_cell_set_color(scols_column_get_header(col), "bold");
	}

	table->scols = tbl;
	table->desc = desc;

	return 0;

unref:
	scols_unref_table(tbl);

	return -errno;
}
