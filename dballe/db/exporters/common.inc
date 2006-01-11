static dba_err set_key(dba_msg dst, int id, dba_record src, dba_keyword key)
{
	dba_var var = dba_record_key_peek(src, key);

	if (var != NULL)
		return dba_msg_set_by_id(dst, var, id);

	return dba_error_ok();
}

static dba_err copy_variable(dba_msg msg, dba_varcode varcode, dba_record rec)
{
	dba_err err = DBA_OK;
	dba_var var = NULL;
	int ltype, l1, l2, pind, p1, p2;

	DBA_RUN_OR_GOTO(cleanup, dba_record_var_enq(rec, varcode, &var));
	DBA_RUN_OR_GOTO(cleanup, dba_record_key_enqi(rec, DBA_KEY_LEVELTYPE, &ltype));
	DBA_RUN_OR_GOTO(cleanup, dba_record_key_enqi(rec, DBA_KEY_L1, &l1));
	DBA_RUN_OR_GOTO(cleanup, dba_record_key_enqi(rec, DBA_KEY_L2, &l2));
	DBA_RUN_OR_GOTO(cleanup, dba_record_key_enqi(rec, DBA_KEY_PINDICATOR, &pind));
	DBA_RUN_OR_GOTO(cleanup, dba_record_key_enqi(rec, DBA_KEY_P1, &p1));
	DBA_RUN_OR_GOTO(cleanup, dba_record_key_enqi(rec, DBA_KEY_P2, &p2));

	DBA_RUN_OR_GOTO(cleanup, dba_msg_set_nocopy(msg, var, ltype, l1, l2, pind, p1, p2));
	var = NULL;

cleanup:
	if (var != NULL)
		dba_var_delete(var);
	return err == DBA_OK ? dba_error_ok() : err;
}

/* vim:set ts=4 sw=4 syntax=c: */
