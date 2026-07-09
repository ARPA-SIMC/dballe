#include <dballe/cmdline/cmdline.h>
#include <dballe/core/csv.h>
#include <dballe/types.h>
#include <dballe/var.h>
#include <wreport/conv.h>
#include <wreport/dtable.h>
#include <wreport/utils/string.h>
#include <wreport/vartable.h>

#include "config.h"
#include <stdlib.h>
#include <string.h>

using namespace dballe;
using namespace dballe::cmdline;
using namespace wreport;
using namespace std;

int op_verbose = 0;

/// Write CSV output to the given output stream
struct FileCSV : CSVWriter
{
    FILE* out;
    FileCSV(FILE* out) : out(out) {}

    void flush_row() override
    {
        fputs(row.c_str(), out);
        putc('\n', out);
        row.clear();
    }
};

struct VarinfoPrinter : public cmdline::Subcommand
{
    int op_csv         = 0;
    int op_crex        = 0;
    CSVWriter* csv_out = nullptr;

    ~VarinfoPrinter() { delete csv_out; }

    void init() override
    {
        if (op_csv)
            csv_out = new FileCSV(stdout);
    }

    void add_to_optable(std::vector<poptOption>& opts) const override
    {
        Subcommand::add_to_optable(opts);
        opts.push_back(poptOption{"csv", 'c', POPT_ARG_NONE,
                                  const_cast<int*>(&op_csv), 0,
                                  "output variables in CSV format", 0});
        opts.push_back(poptOption{"crex", 0, POPT_ARG_NONE,
                                  const_cast<int*>(&op_crex), 0,
                                  "read CREX entries instead of BUFR", 0});
    }

    const Vartable* load_vartable(const char* id)
    {
        if (op_crex)
            return Vartable::get_crex(id);
        else
            return Vartable::get_bufr(id);
    }

    void print_varinfo(Varinfo info) const
    {
        if (csv_out)
            print_varinfo_csv(*csv_out, info);
        else
            print_varinfo_desc(info);
    }

    static std::string format_decimal(Varinfo info)
    {
        string res;
        if (info->scale > 0)
        {
            if (info->len > (unsigned)info->scale)
                for (unsigned i = 0; i < info->len - info->scale; ++i)
                    res += '#';
            res += '.';
            for (unsigned i = 0; i < (unsigned)info->scale; ++i)
                res += '#';
        }
        else if (info->scale < 0)
        {
            for (unsigned i = 0; i < info->len; ++i)
                res += '#';
            for (unsigned i = 0; i < (unsigned)-info->scale; ++i)
                res += '0';
        }
        return res;
    }

    static void print_varinfo_desc(Varinfo info)
    {
        switch (info->type)
        {
            case Vartype::String:
                printf("%d%02d%03d %s [%s, %u characters]\n",
                       WR_VAR_FXY(info->code), info->desc, info->unit,
                       info->len);
                break;
            case Vartype::Binary:
                printf("%d%02d%03d %s [%s, %u bits]\n", WR_VAR_FXY(info->code),
                       info->desc, info->unit, info->bit_len);
                break;
            case Vartype::Integer:
                printf("%d%02d%03d %s [%s, %u digits] range (%d -- %d)\n",
                       WR_VAR_FXY(info->code), info->desc, info->unit,
                       info->len, info->imin, info->imax);
                break;
            case Vartype::Decimal:
                printf("%d%02d%03d %s [%s, %s] range (%g -- %g)\n",
                       WR_VAR_FXY(info->code), info->desc, info->unit,
                       format_decimal(info).c_str(), info->dmin, info->dmax);
                break;
        }
    }

    static void print_varinfo_csv(CSVWriter& out, const Varinfo& info)
    {
        out.add_value(varcode_format(info->code));
        out.add_value(info->desc);
        out.add_value(info->unit);
        out.add_value(vartype_format(info->type));
        switch (info->type)
        {
            case Vartype::String:
                out.add_value(info->len);
                out.add_value_empty();
                out.add_value_empty();
                break;
            case Vartype::Binary:
                out.add_value(info->bit_len);
                out.add_value_empty();
                out.add_value_empty();
                break;
            case Vartype::Integer:
                out.add_value(info->len);
                out.add_value(info->imin);
                out.add_value(info->imax);
                break;
            case Vartype::Decimal:
                out.add_value(format_decimal(info));
                out.add_value(to_string(info->dmin));
                out.add_value(to_string(info->dmax));
                break;
        }
        out.flush_row();
    }
};

struct Cat : public VarinfoPrinter
{
    Cat()
    {
        names.push_back("cat");
        usage = "cat tableid [tableid [...]]";
        desc  = "Output all the contents of a WMO B table.";
    }

    int main(poptContext optCon) override
    {
        const char* item;

        /* Throw away the command name */
        poptGetArg(optCon);

        if (poptPeekArg(optCon) == NULL)
            item = "dballe";
        else
            item = poptGetArg(optCon);

        while (item != NULL)
        {
            const Vartable* table = load_vartable(item);
            table->iterate([&](Varinfo info) {
                print_varinfo(info);
                return true;
            });
            item = poptGetArg(optCon);
        }

        return 0;
    }
};

struct Grep : public VarinfoPrinter
{
    Grep()
    {
        names.push_back("grep");
        usage = "grep string";
        desc = "Output all the contents of the local B table whose description "
               "contains the given string.";
    }

    int main(poptContext optCon) override
    {
        /* Throw away the command name */
        poptGetArg(optCon);

        if (poptPeekArg(optCon) == NULL)
            dba_cmdline_error(optCon,
                              "there should be at least one B or D item to "
                              "expand.  Examples are: B01002 or D03001");

        std::string pattern = str::lower(poptGetArg(optCon));

        const Vartable* table = load_vartable("dballe");
        table->iterate([&](Varinfo info) {
            if (str::lower(info->desc).find(pattern) != string::npos)
                print_varinfo(info);
            return true;
        });

        return 0;
    }
};

#if 0
struct Expand : public cmdline::Subcommand
{
    const Vartable* btable = NULL;
    const DTable* dtable = NULL;

    Expand()
    {
        names.push_back("expand");
        usage = "expand table-entry [table-entry [...]]";
        desc = "Describe a WMO B table entry or expand a WMO D table entry in its components.";
    }

    int main(poptContext optCon) override
    {
        const char* item;

        /* Throw away the command name */
        poptGetArg(optCon);

        btable = Vartable::get("dballe");
        dtable = DTable::get("D000203");

        if (poptPeekArg(optCon) == NULL)
            dba_cmdline_error(optCon, "there should be at least one B or D item to expand.  Examples are: B01002 or D03001");

        while ((item = poptGetArg(optCon)) != NULL)
            expand_table_entry(descriptor_code(item), 0);

        return 0;
    }

    void expand_table_entry(Varcode val, int level)
    {
        int i;
        for (i = 0; i < level; i++)
            printf("\t");

        switch (WR_VAR_F(val))
        {
            case 0:
            {
                Varinfo info = btable->query(val);
                VarinfoPrinter::print_varinfo_desc(info);
                break;
            }
            case 3:
            {
                Opcodes ops = dtable->query(val);

                printf("%d%02d%03d\n", WR_VAR_F(val), WR_VAR_X(val), WR_VAR_Y(val));

                for (size_t i = 0; i < ops.size(); ++i)
                    expand_table_entry(ops[i], level+1);
                break;
            }
            default:
                printf("%d%02d%03d\n", WR_VAR_F(val), WR_VAR_X(val), WR_VAR_Y(val));
        }
    }
};
#endif

struct ExpandCode : public cmdline::Subcommand
{
    ExpandCode()
    {
        names.push_back("expandcode");
        usage = "expandcode varcode [varcode [...]]";
        desc  = "Expand the value of a packed variable code";
    }

    int main(poptContext optCon) override
    {
        const char* item;

        /* Throw away the command name */
        poptGetArg(optCon);
        while ((item = poptGetArg(optCon)) != NULL)
        {
            int code = strtol(item, NULL, 10);
            char c   = 'B';
            switch (WR_VAR_F(code))
            {
                case 0: c = 'B'; break;
                case 1: c = 'R'; break;
                case 2: c = 'C'; break;
                case 3: c = 'D'; break;
            }
            printf("%s: %c%02d%03d\n", item, c, WR_VAR_X(code), WR_VAR_Y(code));
        }

        return 0;
    }
};

#if 0
static const char* table_type = "b";

struct Index : public cmdline::Subcommand
{
    Index()
    {
        names.push_back("index");
        usage = "index [options] filename index-id";
        desc = "Index the contents of a table file";
    }

    void add_to_optable(std::vector<poptOption>& opts) const override
    {
        Subcommand::add_to_optable(opts);
        opts.push_back({ "type", 't', POPT_ARG_STRING, &table_type, 0,
                "format of the table to index ('b', 'd', 'conv')", "type" });
    }

    /**
     * Check that all unit conversions are allowed by dba_uniconv
     *
     * @returns true if all conversions worked, false if some exceptions were thrown
     */
    static bool check_unit_conversions(const char* id)
    {
        const Vartable* othertable = Vartable::get(id);
        bool res = true;
        for (Vartable::const_iterator info = othertable->begin();
                info != othertable->end(); ++info)
        {
            Varcode varcode = info->var;

            /*
            if (var % 1000 == 0)
                fprintf(stderr, "Testing %s %d\n", ids[i], var);
            */

            if (varcode != 0 && !info->is_string())
            {
                try {
                    Varinfo local = varinfo(varcode);
                    convert_units(info->unit, local->unit, 1.0);
                } catch (std::exception& e) {
                    fprintf(stderr, "Checking conversion for var B%02d%03d: %s",
                            WR_VAR_X(varcode), WR_VAR_Y(varcode), e.what());
                    res = false;
                }
            }
        }
        return res;
    }

    int main(poptContext optCon) override
    {
        const char* file;
        const char* id;

        /* Throw away the command name */
        poptGetArg(optCon);
        file = poptGetArg(optCon);
        id = poptGetArg(optCon);
        if (file == NULL)
            dba_cmdline_error(optCon, "input file has not been specified");
        if (id == NULL)
            dba_cmdline_error(optCon, "indexed table ID has not been specified");

        if (strcmp(table_type, "b") == 0)
        {
            if (strcmp(id, "dballe") != 0)
            {
                /* If it's an external table, check unit conversions to DBALLE
                 * correspondents */
                if (!check_unit_conversions(id))
                    fprintf(stderr, "Warning: some variables cannot be converted from %s to dballe\n", id);
            }
        }
        else if (strcmp(table_type, "d") == 0)
            ; /* DBA_RUN_OR_RETURN(bufrex_dtable_index(file, id)); */
        /*
        else if (strcmp(type, "conv") == 0)
            DBA_RUN_OR_RETURN(bufrex_convtable_index_csv(file, id));
        */
        else
            dba_cmdline_error(optCon, "'%s' is not a valid table type", table_type);

        return 0;
    }
};
#endif

struct Describe : public cmdline::Subcommand
{
    Describe()
    {
        names.push_back("describe");
        usage    = "describe [options] what [values]";
        desc     = "Invoke the formatter to describe the given values";
        longdesc = "Supported so far are: \"level ltype l1 l2\", \"trange pind "
                   "p1 p2\"";
    }

    int main(poptContext optCon) override
    {
        const char* what;

        /* Throw away the command name */
        poptGetArg(optCon);
        if ((what = poptGetArg(optCon)) == NULL)
            dba_cmdline_error(optCon,
                              "you need to specify what you want to describe.  "
                              "Available options are: 'level' and 'trange'");

        if (strcmp(what, "level") == 0)
        {
            const char* sltype1 = poptGetArg(optCon);
            const char* sl1     = poptGetArg(optCon);
            const char* sltype2 = poptGetArg(optCon);
            const char* sl2     = poptGetArg(optCon);
            if (sltype1 == NULL)
                dba_cmdline_error(optCon,
                                  "you need provide 1, 2, 3 or 4 numbers that "
                                  "identify the level or layer");
            string formatted =
                Level(strtoul(sltype1, NULL, 10),
                      sl1 == NULL ? 0 : strtoul(sl1, NULL, 10),
                      sltype2 == NULL ? 0 : strtoul(sltype2, NULL, 10),
                      sl2 == NULL ? 0 : strtoul(sl2, NULL, 10))
                    .describe();
            puts(formatted.c_str());
        }
        else if (strcmp(what, "trange") == 0)
        {
            const char* sptype = poptGetArg(optCon);
            const char* sp1    = poptGetArg(optCon);
            const char* sp2    = poptGetArg(optCon);
            if (sptype == NULL)
                dba_cmdline_error(optCon, "you need provide 1, 2 or 3 numbers "
                                          "that identify the time range");
            string formatted = Trange(strtoul(sptype, NULL, 10),
                                      sp1 == NULL ? 0 : strtoul(sp1, NULL, 10),
                                      sp2 == NULL ? 0 : strtoul(sp2, NULL, 10))
                                   .describe();
            puts(formatted.c_str());
        }
        else
            dba_cmdline_error(optCon,
                              "cannot handle %s.  Available options are: "
                              "'level' and 'trange'.",
                              what);

        return 0;
    }
};

int main(int argc, const char* argv[])
{
    Command dbatbl;
    dbatbl.name     = "dbatbl";
    dbatbl.desc     = "Manage on-disk reference tables for DB-ALLe";
    dbatbl.longdesc = "This tool allows to index and query the tables that are "
                      "needed for normal functioning of DB-ALLe";

    dbatbl.add_subcommand(new Cat);
    dbatbl.add_subcommand(new Grep);
    // dbatbl.add_subcommand(new Expand);
    dbatbl.add_subcommand(new ExpandCode);
    // dbatbl.add_subcommand(new Index);
    dbatbl.add_subcommand(new Describe);

    return dbatbl.main(argc, argv);
}
