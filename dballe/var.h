#ifndef DBALLE_VAR_H
#define DBALLE_VAR_H

/**
 * @file
 *
 * Create wreport variables from the DB-All.e B table
 */

#include <wreport/var.h>
#include <memory>
#include <string>

namespace dballe {

/// Return a Varinfo entry from the DB-All.e B table
wreport::Varinfo varinfo(wreport::Varcode code);

/// Return a Varinfo entry from the DB-All.e B table
wreport::Varinfo varinfo(const char* code);

/// Return a Varinfo entry from the DB-All.e B table
wreport::Varinfo varinfo(const std::string& code);


/**
 * Resolve a variable name to a varcode proper, dealing with aliases and
 * validation
 */
wreport::Varcode resolve_varcode(const char* name);

/**
 * Resolve a variable name to a varcode proper, dealing with aliases and
 * validation
 */
wreport::Varcode resolve_varcode(const std::string& name);


/// Create a new Var, from the DB-All.e B table, with undefined value
template<typename C>
static inline wreport::Var var(C code) { return wreport::Var(varinfo(code)); }

/// Create a new Var, from the DB-All.e B table, with value
template<typename C, typename T>
static inline wreport::Var var(C code, const T& val) { return wreport::Var(varinfo(code), val); }

/// Create a new Var, as a copy of an existing variable
static inline std::unique_ptr<wreport::Var> newvar(const wreport::Var& var)
{
    return std::unique_ptr<wreport::Var>(new wreport::Var(var));
}

/// Create a new Var, from the DB-All.e B table, with undefined value
template<typename C>
static inline std::unique_ptr<wreport::Var> newvar(C code)
{
    return std::unique_ptr<wreport::Var>(new wreport::Var(varinfo(code)));
}

/// Create a new Var, from the DB-All.e B table, with value
template<typename C, typename T>
std::unique_ptr<wreport::Var> newvar(C code, const T& val)
{
    return std::unique_ptr<wreport::Var>(new wreport::Var(varinfo(code), val));
}

}

#endif
