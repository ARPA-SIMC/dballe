// List of headers to precompile.
// See
// https://github.com/mesonbuild/meson/blob/master/docs/markdown/Precompiled-headers.md
//
// Hint: git grep -h '#include <'| sort | uniq -c | sort -n
//
// If you wish to compile your project without precompiled headers, you can
// change the value of the pch option by passing -Db_pch=false argument to
// Meson at configure time or later with meson configure.
//
// Note that adding a header here will have it always included when compiling.
// Turn off precompiled headers to check include consistency.
//
// Note also that changing one of the included headers will trigger a rebuild
// of everything.

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iosfwd>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_set>
#include <vector>
#include <wreport/bulletin.h>
#include <wreport/codetables.h>
#include <wreport/conv.h>
#include <wreport/error.h>
#include <wreport/notes.h>
#include <wreport/subset.h>
#include <wreport/utils/string.h>
#include <wreport/utils/sys.h>
#include <wreport/var.h>
#include <wreport/varinfo.h>
#include <wreport/vartable.h>
