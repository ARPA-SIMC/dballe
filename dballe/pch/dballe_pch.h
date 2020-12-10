// List of headers to precompile.
// See https://github.com/mesonbuild/meson/blob/master/docs/markdown/Precompiled-headers.md
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

#include <cstring>
#include <memory>
#include <vector>
#include <cstdlib>
#include <wreport/bulletin.h>
#include <string>
#include <wreport/error.h>
#include <functional>
#include <cstdio>
#include <wreport/var.h>
#include <sstream>
#include <algorithm>
#include <wreport/varinfo.h>
#include <iostream>
#include <cmath>
#include <wreport/notes.h>
#include <wreport/subset.h>
#include <unistd.h>
#include <map>
#include <wreport/utils/string.h>
#include <ostream>
#include <iosfwd>
#include <wreport/vartable.h>
#include <set>
#include <wreport/conv.h>
#include <unordered_set>
#include <limits>
#include <cstdarg>
#include <cctype>
#include <wreport/utils/sys.h>
#include <wreport/codetables.h>
#include <sys/types.h>
#include <stdexcept>
#include <iomanip>
#include <cassert>
