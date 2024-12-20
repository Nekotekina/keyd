#include "keyd.h"
#include "macro.h"
#include "config.h"
#include <ranges>
#include <numeric>
#include <span>

/*
 * Parses expressions of the form: C-t hello enter.
 * Returns 0 on success.
 */

int macro_parse(std::string_view s, macro& macro, struct config* config)
{
	#define ADD_ENTRY(t, d) macro.push_back(macro_entry{.type = t, .data = static_cast<uint16_t>(d)})

	macro.clear();
	while (true) {
		std::string_view tok = s.substr(0, s.find_first_of(" \t\r\n"));
		std::string buf;
		if (tok.starts_with("cmd(")) {
			s.remove_prefix(4);
			for (size_t i = 0; i < s.size(); i++) {
				if (s[i] == '\\')
					i++;
				else if (s[i] == ')') {
					tok = s.substr(0, i);
					break;
				}
			}
			if (tok.size() == s.size()) {
				err("incomplete macro command found");
				return -1;
			}
			if (!config) {
				err("commands are not allowed in this context");
				return -1;
			}
			if (config->commands.size() >= std::numeric_limits<decltype(descriptor_arg::idx)>::max()) {
				err("max commands exceeded");
				return -1;
			}
			buf.assign(tok);
			buf.resize(str_escape(buf.data()));
			ADD_ENTRY(MACRO_COMMAND, config->commands.size());
			config->commands.emplace_back(std::move(buf));
			s.remove_prefix(tok.size() + 1);
			s = s.substr(s.find_first_not_of(" \t\r\n") + 1);
			continue;
		}

		s.remove_prefix(tok.size());
		buf.resize(str_escape(buf.data()));
		tok = buf;
		uint8_t code, mods;

		if (!parse_key_sequence(tok, &code, &mods)) {
			ADD_ENTRY(MACRO_KEYSEQUENCE, (mods << 8) | code);
		} else if (tok.find_first_of('+') + 1) {
			for (std::span<const char> range : std::ranges::split_view(tok, '+')) {
				const std::string_view key(range.data(), range.size());

				if (key.ends_with("ms") && key.find_first_not_of("0123456789") == key.size() - 2)
					ADD_ENTRY(MACRO_TIMEOUT, atoi(key.data()));
				else if (!parse_key_sequence(key, &code, &mods))
					ADD_ENTRY(MACRO_HOLD, code);
				else {
					err("%s is not a valid key", std::string(key).c_str());
					return -1;
				}
			}

			ADD_ENTRY(MACRO_RELEASE, 0);
		} else if (tok.ends_with("ms") && tok.find_first_not_of("0123456789") == tok.size() - 2) {
			ADD_ENTRY(MACRO_TIMEOUT, atoi(tok.data()));
		} else {
			uint32_t codepoint;
			while (int chrsz = utf8_read_char(tok, codepoint)) {
				int i;
				int xcode;

				if (chrsz == 1 && codepoint < 128) {
					for (i = 0; i < 256; i++) {
						const char *name = keycode_table[i].name;
						const char *shiftname = keycode_table[i].shifted_name;

						if (name && name[0] == tok[0] && name[1] == 0) {
							ADD_ENTRY(MACRO_KEYSEQUENCE, i);
							break;
						}

						if (shiftname && shiftname[0] == tok[0] && shiftname[1] == 0) {
							ADD_ENTRY(MACRO_KEYSEQUENCE, (MOD_SHIFT << 8) | i);
							break;
						}
					}
				} else if ((xcode = unicode_lookup_index(codepoint)) > 0)
					ADD_ENTRY(MACRO_UNICODE, xcode);

				if (tok.size() <= size_t(chrsz))
					break;
				tok.remove_prefix(chrsz);
			}
		}

		if (s.empty())
			break;
		s = s.substr(s.find_first_not_of(" \t\r\n"));
	}

	return 0;

	#undef ADD_ENTRY
}

void macro_execute(void (*output)(uint8_t, uint8_t),
		   const macro& macro, size_t timeout, struct config* config)
{
	size_t i;
	int hold_start = -1;

	for (i = 0; i < macro.size(); i++) {
		const macro_entry *ent = &macro[i];

		switch (ent->type) {
			size_t j;
			uint16_t idx;
			uint8_t codes[4];
			uint8_t code, mods;

		case MACRO_HOLD:
			if (hold_start == -1)
				hold_start = i;

			output(ent->data, 1);

			break;
		case MACRO_RELEASE:
			if (hold_start != -1) {
				size_t j;

				for (j = hold_start; j < i; j++) {
					const struct macro_entry *ent = &macro[j];
					output(ent->data, 0);
				}

				hold_start = -1;
			}
			break;
		case MACRO_UNICODE:
			idx = ent->data;

			unicode_get_sequence(idx, codes);

			for (j = 0; j < 4; j++) {
				output(codes[j], 1);
				output(codes[j], 0);
			}

			break;
		case MACRO_KEYSEQUENCE:
			code = ent->data;
			mods = ent->data >> 8;

			for (j = 0; j < ARRAY_SIZE(modifiers); j++) {
				uint8_t code = modifiers[j].key;
				uint8_t mask = modifiers[j].mask;

				if (mods & mask)
					output(code, 1);
			}

			if (mods && timeout)
				usleep(timeout);

			output(code, 1);
			output(code, 0);

			for (j = 0; j < ARRAY_SIZE(modifiers); j++) {
				uint8_t code = modifiers[j].key;
				uint8_t mask = modifiers[j].mask;

				if (mods & mask)
					output(code, 0);
			}


			break;
		case MACRO_TIMEOUT:
			usleep(ent->data * 1E3);
			break;
		case MACRO_COMMAND:
			extern void execute_command(const char *cmd);
			if (config)
				execute_command(config->commands.at(ent->data).c_str());
			break;
		}

		if (timeout)
			usleep(timeout);
	}
}
