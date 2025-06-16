// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../poseidon/mysql/mysql_table_structure.hpp"
using namespace ::poseidon;

int
main()
  {
    MySQL_Table_Structure table;
    MySQL_Table_Column col;
    MySQL_Table_Index ind;

    POSEIDON_TEST_CHECK(table.count_columns() == 0);
    POSEIDON_TEST_CHECK(table.count_indexes() == 0);

    table.set_name(&"table_1");
    POSEIDON_TEST_CHECK_CATCH(table.set_name(&""));
    POSEIDON_TEST_CHECK_CATCH(table.set_name(&"invalid.name"));
    POSEIDON_TEST_CHECK(table.name() == "table_1");

    // -- columns
    col.name = &"int_1";
    col.type = mysql_column_int;
    POSEIDON_TEST_CHECK(table.add_column(col) == 0);

    col.name = &"varchar_2";
    col.type = mysql_column_varchar;
    POSEIDON_TEST_CHECK(table.add_column(col) == 1);

    col.name = &"blob_3";
    col.type = mysql_column_blob;
    POSEIDON_TEST_CHECK(table.add_column(col) == 2);

    col.name = &"inT_1";
    col.type = mysql_column_int64;
    POSEIDON_TEST_CHECK(table.add_column(col) == 0);

    POSEIDON_TEST_CHECK(table.count_columns() == 3);
    POSEIDON_TEST_CHECK(table.column(0).name == "inT_1");
    POSEIDON_TEST_CHECK(table.column(0).type == mysql_column_int64);
    POSEIDON_TEST_CHECK(table.column(1).name == "varchar_2");
    POSEIDON_TEST_CHECK(table.column(1).type == mysql_column_varchar);
    POSEIDON_TEST_CHECK(table.column(2).name == "blob_3");
    POSEIDON_TEST_CHECK(table.column(2).type == mysql_column_blob);

    // -- indexes
    ind.name = &"primary";
    ind.type = mysql_index_multi;
    ind.columns.clear();
    ind.columns.emplace_back(&"int_1");
    POSEIDON_TEST_CHECK_CATCH(table.add_index(ind));

    ind.type = mysql_index_unique;
    POSEIDON_TEST_CHECK(table.add_index(ind) == 0);

    ind.name = &"index_2";
    ind.type = mysql_index_multi;
    ind.columns.clear();
    POSEIDON_TEST_CHECK_CATCH(table.add_index(ind));

    ind.columns.clear();
    ind.columns.emplace_back(&"nonexistent");
    POSEIDON_TEST_CHECK_CATCH(table.add_index(ind));

    ind.columns.clear();
    ind.columns.emplace_back(&"blob_3");
    ind.columns.emplace_back(&"int_1");
    POSEIDON_TEST_CHECK(table.add_index(ind) == 1);

    ind.name = &"PrImArY";
    ind.type = mysql_index_unique;
    ind.columns.clear();
    ind.columns.emplace_back(&"varchar_2");
    POSEIDON_TEST_CHECK(table.add_index(ind) == 0);

    POSEIDON_TEST_CHECK(table.count_indexes() == 2);
    POSEIDON_TEST_CHECK(table.index(0).name == "PrImArY");
    POSEIDON_TEST_CHECK(table.index(0).type == mysql_index_unique);
    POSEIDON_TEST_CHECK(table.index(0).columns.size() == 1);
    POSEIDON_TEST_CHECK(table.index(0).columns.at(0) == "varchar_2");
    POSEIDON_TEST_CHECK(table.index(1).name == "index_2");
    POSEIDON_TEST_CHECK(table.index(1).type == mysql_index_multi);
    POSEIDON_TEST_CHECK(table.index(1).columns.size() == 2);
    POSEIDON_TEST_CHECK(table.index(1).columns.at(0) == "blob_3");
    POSEIDON_TEST_CHECK(table.index(1).columns.at(1) == "int_1");

    // -- clear
    table.clear_columns();

    POSEIDON_TEST_CHECK(table.count_columns() == 0);
    POSEIDON_TEST_CHECK(table.count_indexes() == 0);
  }
