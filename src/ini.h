/*
 * keyd - A key remapping daemon.
 *
 * © 2019 Raheman Vaiya (see also: LICENSE).
 */
#ifndef INI_H
#define INI_H

#include <stdint.h>
#include <vector>
#include <string>

struct ini_entry {
	char *key;
	char *val;

	size_t lnum;		// The line number in the original source file.
};

struct ini_section {
	size_t lnum;
	std::string name;
	std::vector<ini_entry> entries;
};

using ini = std::vector<ini_section>;

/*
 * Reads a string of the form:
 *
 *  [section]
 *
 *  # Comment
 *
 *  key1 = val1
 *  key2 = val2
 *  key3
 *
 *  [section2]
 *  ...
 *
 *  Stripping comments and empty lines along the way.
 *  Each entry is a non comment, non empty line
 *  sripped of leading whitespace. If a default
 *  section name is supplied then entries not
 *  listed under an explicit heading will be
 *  returned under the named section. If
 *  no value is specified, val is NULL in
 *  the corresponding entry.
 *
 *  The returned result is statically allocated and only
 *  valid until the next invocation. It should not be
 *  freed.
 */

ini ini_parse_string(char *s, const char *default_section_name);

void parse_kvp(char *s, char **key, char **value);

#endif
