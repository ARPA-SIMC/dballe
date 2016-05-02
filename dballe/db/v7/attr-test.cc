            Var var(varinfo(WR_VAR(0, 1, 2)), 234);
            vars.add(&var, lt2->second);
            da->insert(*t, vars, bulk::ERROR);
        }
        t->commit();
    }

    void test_setup()
    {
        V7DriverFixture::test_setup();
        attr = driver->create_attr();
        reset_attr();
    }

    Var query(int id_data, unsigned expected_attr_count)
    {
        Var res(varinfo(WR_VAR(0, 12, 101)));
        unsigned count = 0;
        attr->read(id_data, [&](unique_ptr<Var> attr) { res.seta(move(attr)); ++count; });
        wassert(actual(count) == expected_attr_count);
        return res;
    }
};

class Tests : public FixtureTestCase<Fixture>
{
    using FixtureTestCase::FixtureTestCase;

    void register_tests() override
    {
        add_method("insert", [](Fixture& f) {
            using namespace dballe::db::v7;
            auto& at = *f.attr;

            auto conn_t = f.conn->transaction();
            unique_ptr<dballe::db::v7::Transaction> t(new dballe::db::v7::Transaction(move(conn_t)));

            Var var1(varinfo(WR_VAR(0, 12, 101)), 280.0);
            var1.seta(newvar(WR_VAR(0, 33, 7), 50));

            Var var2(varinfo(WR_VAR(0, 12, 101)), 280.0);
            var2.seta(newvar(WR_VAR(0, 33, 7), 75));

            // Insert two attributes
            {
                bulk::InsertAttrsV7 attrs(t->state.values_new);
                attrs.add_all(var1, 1);
                at.insert(*t, attrs, Attr::ERROR);
                wassert(actual(attrs.size()) == 1);
                wassert(actual(attrs[0].needs_insert()).isfalse());
                wassert(actual(attrs[0].inserted()).istrue());
                wassert(actual(attrs[0].needs_update()).isfalse());
                wassert(actual(attrs[0].updated()).isfalse());
            }
            {
                bulk::InsertAttrsV7 attrs(t->state.values_new);
                attrs.add_all(var2, 2);
                at.insert(*t, attrs, Attr::ERROR);
                wassert(actual(attrs.size()) == 1);
                wassert(actual(attrs[0].needs_insert()).isfalse());
                wassert(actual(attrs[0].inserted()).istrue());
                wassert(actual(attrs[0].needs_update()).isfalse());
                wassert(actual(attrs[0].updated()).isfalse());
            }

            // Reinsert the first attribute: it should work, doing no insert/update queries
            {
                bulk::InsertAttrsV7 attrs(t->state.values_new);
                attrs.add_all(var1, 1);
                at.insert(*t, attrs, Attr::IGNORE);
                wassert(actual(attrs.size()) == 1);
                wassert(actual(attrs[0].needs_insert()).isfalse());
                wassert(actual(attrs[0].inserted()).isfalse());
                wassert(actual(attrs[0].needs_update()).isfalse());
                wassert(actual(attrs[0].updated()).isfalse());
            }

            // Reinsert the second attribute: it should work, doing no insert/update queries
            {
                bulk::InsertAttrsV7 attrs(t->state.values_new);
                attrs.add_all(var2, 2);
                at.insert(*t, attrs, Attr::UPDATE);
                wassert(actual(attrs.size()) == 1);
                wassert(actual(attrs[0].needs_insert()).isfalse());
                wassert(actual(attrs[0].inserted()).isfalse());
                wassert(actual(attrs[0].needs_update()).isfalse());
                wassert(actual(attrs[0].updated()).isfalse());
            }

            // Load the attributes for the first variable
            {
                Var var(f.query(1, 1));
                wassert(actual(var.next_attr()->code()) == WR_VAR(0, 33, 7));
                wassert(actual(*var.next_attr()) == 50);
            }

            // Load the attributes for the second variable
            {
                Var var(f.query(2, 1));
                wassert(actual(var.next_attr()->code()) == WR_VAR(0, 33, 7));
                wassert(actual(*var.next_attr()) == 75);
            }

            // Update both values
            {
                bulk::InsertAttrsV7 attrs(t->state.values_new);
                attrs.add_all(var2, 1);
                at.insert(*t, attrs, Attr::UPDATE);
                wassert(actual(attrs.size()) == 1);
                wassert(actual(attrs[0].needs_insert()).isfalse());
                wassert(actual(attrs[0].inserted()).isfalse());
                wassert(actual(attrs[0].needs_update()).isfalse());
                wassert(actual(attrs[0].updated()).istrue());
            }
            {
                bulk::InsertAttrsV7 attrs(t->state.values_new);
                attrs.add_all(var1, 2);
                at.insert(*t, attrs, Attr::UPDATE);
                wassert(actual(attrs.size()) == 1);
                wassert(actual(attrs[0].needs_insert()).isfalse());
                wassert(actual(attrs[0].inserted()).isfalse());
                wassert(actual(attrs[0].needs_update()).isfalse());
                wassert(actual(attrs[0].updated()).istrue());
            }
            // Load the attributes again to verify that they changed
            {
                Var var(f.query(1, 1));
                wassert(actual(var.next_attr()->code()) == WR_VAR(0, 33, 7));
                wassert(actual(*var.next_attr()) == 75);
            }
            {
                Var var(f.query(2, 1));
                wassert(actual(var.next_attr()->code()) == WR_VAR(0, 33, 7));
                wassert(actual(*var.next_attr()) == 50);
            }

            // TODO: test a mix of update and insert
        });
    }
};

Tests tg1("db_sql_attr_v7_sqlite", "SQLITE", db::V7);
#ifdef HAVE_LIBPQ
Tests tg3("db_sql_attr_v7_postgresql", "POSTGRESQL", db::V7);
#endif
#ifdef HAVE_MYSQL
Tests tg4("db_sql_attr_v7_mysql", "MYSQL", db::V7);
#endif

}
