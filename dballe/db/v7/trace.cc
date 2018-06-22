#include "trace.h"

namespace dballe {
namespace db {
namespace v7 {

void ProfileTrace::print(FILE* out)
{
    fprintf(stderr, "Transaction end: %u queries\n", profile_count_select + profile_count_insert + profile_count_update + profile_count_delete);
    fprintf(stderr, "   %u selects, %u rows\n", profile_count_select, profile_count_select_rows);
    fprintf(stderr, "   %u inserts, %u rows\n", profile_count_insert, profile_count_insert_rows);
    fprintf(stderr, "   %u updates, %u rows\n", profile_count_update, profile_count_update_rows);
    fprintf(stderr, "   %u deletes\n", profile_count_delete);
}

void ProfileTrace::reset()
{
    profile_count_select = 0;
    profile_count_insert = 0;
    profile_count_update = 0;
    profile_count_delete = 0;
    profile_count_select_rows = 0;
    profile_count_insert_rows = 0;
    profile_count_update_rows = 0;
}

void QuietProfileTrace::print(FILE* out)
{
}

}
}
}
