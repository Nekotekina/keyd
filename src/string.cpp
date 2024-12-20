/*
 * keyd - A key remapping daemon.
 *
 * © 2019 Raheman Vaiya (see also: LICENSE).
 */

#include "strutil.h"


int utf8_read_char(std::string_view _s, uint32_t &code)
{
	if (_s.empty())
		return 0;
	const auto s = reinterpret_cast<const unsigned char*>(_s.data());

	if (s[0] >= 0xF0) {
		if (_s.size() < 4)
			return 0;
		code = (s[0] & 0x07) << 18 | (s[1] & 0x3F) << 12 | (s[2] & 0x3F) << 6 | (s[3] & 0x3F);
		return 4;
	} else if (s[0] >= 0xE0) {
		if (_s.size() < 3)
			return 0;
		code = (s[0] & 0x0F) << 12 | (s[1] & 0x3F) << 6 | (s[2] & 0x3F);
		return 3;
	} else if (s[0] >= 0xC0) {
		if (_s.size() < 2)
			return 0;
		code = (s[0] & 0x1F) << 6 | (s[1] & 0x3F);
		return 2;
	} else {
		code = s[0] & 0x7F;
		return 1;
	}
}

int utf8_read_char(const char *_s, uint32_t *code)
{
	return utf8_read_char(_s, *code);
}

int utf8_strlen(std::string_view s)
{
	uint32_t code;
	int n = 0;

	while (int csz = utf8_read_char(s, code)) {
		n++;
		s.remove_prefix(csz);
	}

	return n;
}

/*
 * Returns the character size in bytes, or 0 in the case of the empty string.
 */
size_t str_escape(char *s)
{
	size_t n = 0;

	for (size_t i = 0; s[i]; i++) {
		if (s[i] == '\\') {
			switch (s[i+1]) {
			case 'n':
				s[n++] = '\n';
				break;
			case 't':
				s[n++] = '\t';
				break;
			case '\\':
				s[n++] = '\\';
				break;
			case ')':
				s[n++] = ')';
				break;
			case '(':
				s[n++] = '(';
				break;
			case 0:
				s[n] = 0;
				return n;
			default:
				s[n++] = '\\';
				s[n++] = s[i+1];
				break;
			}

			i++;
		} else {
			s[n++] = s[i];
		}
	}

	s[n] = 0;
	return n;
}
