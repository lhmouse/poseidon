// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../poseidon/mysql/mysql_table_structure.hpp"
#include "../poseidon/mysql/enums.hpp"
using namespace ::poseidon;

int
main()
  {
    MySQL_Table_Structure table;
    MySQL_Table_Structure::Column col;
    MySQL_Table_Structure::Index ind;

    POSEIDON_TEST_CHECK(table.count_columns() == 0);
    POSEIDON_TEST_CHECK(table.count_indexes() == 0);

    table.set_name(sref("table_1"));
    POSEIDON_TEST_CHECK_CATCH(table.set_name(sref("")));
    POSEIDON_TEST_CHECK_CATCH(table.set_name(sref("invalid.name")));
    POSEIDON_TEST_CHECK(table.name() == sref("table_1"));

    // -- columns
    col.name = sref("int_1");
    col.type = mysql_column_int;
    POSEIDON_TEST_CHECK(table.add_column(col) == 0);

    col.name = sref("varchar_2");
    col.type = mysql_column_varchar;
    POSEIDON_TEST_CHECK(table.add_column(col) == 1);

    col.name = sref("blob_3");
    col.type = mysql_column_blob;
    POSEIDON_TEST_CHECK(table.add_column(col) == 2);

    col.name = sref("inT_1");
    col.type = mysql_column_int64;
    POSEIDON_TEST_CHECK(table.add_column(col) == 0);

    POSEIDON_TEST_CHECK(table.count_columns() == 3);
    POSEIDON_TEST_CHECK(table.column(0).name == sref("inT_1"));
    POSEIDON_TEST_CHECK(table.column(0).type == mysql_column_int64);
    POSEIDON_TEST_CHECK(table.column(1).name == sref("varchar_2"));
    POSEIDON_TEST_CHECK(table.column(1).type == mysql_column_varchar);
    POSEIDON_TEST_CHECK(table.column(2).name == sref("blob_3"));
    POSEIDON_TEST_CHECK(table.column(2).type == mysql_column_blob);

    // -- indexes
    ind.name = sref("primary");
    ind.columns.clear();
    ind.columns.emplace_back(sref("int_1"));
    ind.unique = false;
    POSEIDON_TEST_CHECK_CATCH(table.add_index(ind));

    ind.unique = true;
    POSEIDON_TEST_CHECK(table.add_index(ind) == 0);

    ind.name = sref("index_2");
    ind.columns.clear();
    ind.unique = false;
    POSEIDON_TEST_CHECK_CATCH(table.add_index(ind));

    ind.columns.clear();
    ind.columns.emplace_back(sref("nonexistent"));
    POSEIDON_TEST_CHECK_CATCH(table.add_index(ind));

    ind.columns.clear();
    ind.columns.emplace_back(sref("blob_3"));
    ind.columns.emplace_back(sref("int_1"));
    POSEIDON_TEST_CHECK(table.add_index(ind) == 1);

    ind.name = sref("PrImArY");
    ind.columns.clear();
    ind.columns.emplace_back(sref("varchar_2"));
    ind.unique = true;
    POSEIDON_TEST_CHECK(table.add_index(ind) == 0);

    POSEIDON_TEST_CHECK(table.count_indexes() == 2);
    POSEIDON_TEST_CHECK(table.index(0).name == sref("PrImArY"));
    POSEIDON_TEST_CHECK(table.index(0).unique == true);
    POSEIDON_TEST_CHECK(table.index(0).columns.size() == 1);
    POSEIDON_TEST_CHECK(table.index(0).columns.at(0) == sref("varchar_2"));
    POSEIDON_TEST_CHECK(table.index(1).name == sref("index_2"));
    POSEIDON_TEST_CHECK(table.index(1).unique == false);
    POSEIDON_TEST_CHECK(table.index(1).columns.size() == 2);
    POSEIDON_TEST_CHECK(table.index(1).columns.at(0) == sref("blob_3"));
    POSEIDON_TEST_CHECK(table.index(1).columns.at(1) == sref("int_1"));

    // -- clear
    table.clear_columns();

    POSEIDON_TEST_CHECK(table.count_columns() == 0);
    POSEIDON_TEST_CHECK(table.count_indexes() == 0);
  }
