/*
 * keyd - A key remapping daemon.
 *
 * © 2019 Raheman Vaiya (see also: LICENSE).
 */
#ifndef STRUTIL_H
#define STRUTIL_H

#include <stdint.h>
#include <stdlib.h>
#include <string_view>
#include <algorithm>

template <unsigned Pat = -1u>
struct split_str;

struct split_sentinel{
	template <unsigned C>
	constexpr bool operator !=(const split_str<C>& rhs) const
	{
		return rhs.pat;
	}

	template <unsigned C>
	constexpr operator split_str<C>() const
	{
		return {.pat = nullptr};
	}
};

template <unsigned Pat>
struct split_str {
	const char* pat = nullptr;
	std::string_view str;

	using difference_type = ssize_t;
	using value_type = std::string_view;
	using pointer = void;
	using reference = std::string_view;
	using iterator_category = std::forward_iterator_tag;

	constexpr auto pattern() const
	{
		if constexpr (Pat <= 255)
			return Pat;
		else
			return pat;
	}

	constexpr split_str begin() const
	{
		return *this;
	}

	static constexpr split_sentinel end()
	{
		return {};
	}

	constexpr std::string_view operator*() const
	{
		return this->str.substr(0, this->str.find_first_of(this->pattern()));
	}

	constexpr bool operator !=(const split_sentinel&) const
	{
		return this->pat;
	}

	constexpr bool operator <=>(const split_str&) const = default;

	constexpr ssize_t count() const
	{
		if (!this->pat)
			return 0;
		else if constexpr (Pat <= 255)
			return std::count(this->str.begin(), this->str.end(), Pat) + 1;
		else
			return std::count_if(this->str.begin(), this->str.end(), [pat = std::string_view(this->pat)](char c) { return pat.find_first_of(c) + 1; }) + 1;
	}

	constexpr ssize_t operator -(const split_str& rhs) const
	{
		return this->pat ? rhs.count() - this->count() : rhs.count();
	}

	constexpr split_str& operator++()
	{
		this->str.remove_prefix((**this).size());
		if (!this->str.empty()) {
			this->str.remove_prefix(1);
		} else {
			this->pat = nullptr;
		}

		return *this;
	}
};

template <unsigned Ch>
static constexpr split_str<Ch> split_char(std::string_view str)
{
	static_assert(Ch <= 255);
	return {.pat = "", .str = str};
}

static constexpr split_str<-1u> split_chars(std::string_view str, const char* pat)
{
	return {.pat = pat, .str = str};
}

int utf8_read_char(const char *_s, uint32_t *code);
int utf8_read_char(std::string_view s, uint32_t& code);
int utf8_strlen(std::string_view s);

size_t str_escape(char *s);
#endif
