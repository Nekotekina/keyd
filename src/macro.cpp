#include "keyd.h"
#include "macro.h"
#include "config.h"

/*
 * Parses expressions of the form: C-t type(hello) enter
 * Returns 0 on success.
 */

int macro_parse(std::string_view s, macro& macro, struct config* config)
{
	std::vector<macro_entry> entries;

	#define ADD_ENTRY(t, d) entries.emplace_back(macro_entry{.type = t, .id = static_cast<uint16_t>(d), .code = static_cast<uint16_t>(d)})

	std::string buf;
	while (!(s = s.substr(std::min(s.size(), s.find_first_not_of(C_SPACES)))).empty()) {
		std::string_view tok = s.substr(0, s.find_first_of(C_SPACES));
		const bool is_txt = tok.starts_with("type(") || tok.starts_with("text(") || tok.starts_with("txt(") || tok.starts_with("t(");
		const bool is_cmd = tok.starts_with("cmd(") || tok.starts_with("command(");
		if (is_txt || is_cmd) {
			s.remove_prefix(tok.find_first_of('(') + 1);
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
			if (is_cmd && config->commands.size() > INT16_MAX) {
				err("max commands exceeded");
				return -1;
			}
			buf.assign(tok);
			buf.resize(str_escape(buf.data()));
			s.remove_prefix(tok.size() + 1);
			if (is_cmd) {
				ADD_ENTRY(MACRO_COMMAND, config->commands.size());
				config->commands.emplace_back(::ucmd{
					.cmd = std::move(buf),
					.env = config->cmd_env,
				});
			} else {
				uint32_t codepoint;
				while (int chrsz = utf8_read_char(tok, codepoint)) {
					if (chrsz == 1 && codepoint < 128) {
						size_t i = 0;
						for (i = 1; i < KEYD_ENTRY_COUNT; i++) {
							const auto name = keycode_table[i].name();
							const char* altname = keycode_table[i].alt_name;
							const char *shiftname = keycode_table[i].shifted_name;

							if (name.size() == 1 && name[0] == tok[0]) {
								ADD_ENTRY(MACRO_KEY_TAP, i).mods = {};
								break;
							}

							if (shiftname && shiftname[0] == tok[0] && shiftname[1] == 0) {
								ADD_ENTRY(MACRO_KEY_TAP, i).mods = { .mods = (1 << MOD_SHIFT), .wildc = 0 };
								break;
							}

							if (altname && altname[0] == tok[0] && altname[1] == 0) {
								ADD_ENTRY(MACRO_KEY_TAP, i).mods = {};
								break;
							}
						}
						if (i == KEYD_ENTRY_COUNT) {
							break;
						}
					} else {
						ADD_ENTRY(MACRO_UNICODE, codepoint).id = codepoint >> 16;
					}

					tok.remove_prefix(chrsz);
				}
				if (!tok.empty()) {
					err("invalid macro text found: %.*s", (int)tok.size(), tok.data());
					return -1;
				}
			}
			continue;
		}

		s.remove_prefix(tok.size());
		buf = tok;
		buf.resize(str_escape(buf.data()));
		tok = buf;
		uint8_t mods;
		uint8_t wildc;
		uint16_t code;

		if (parse_key_sequence(tok, &code, &mods, &wildc) == 0 && code) {
			if (wildc) {
				// Wildcard is only allowed in single-key macro
				err("%.*s has a wildcard inside a macro", (int)tok.size(), tok.data());
				return -1;
			}
			entries.emplace_back(macro_entry{
				.type = MACRO_KEY_TAP,
				.id = code,
				.mods = { .mods = mods, .wildc = 0 },
			});
			continue;
		} else if (tok.find_first_of('+') + 1) {
			for (auto key : split_char<'+'>(tok)) {
				if (key.ends_with("ms") && key.find_first_not_of("0123456789") == key.size() - 2)
					ADD_ENTRY(MACRO_TIMEOUT, atoi(key.data()));
				else if (parse_key_sequence(key, &code, &mods, &wildc) == 0 && code && !mods && !wildc)
					ADD_ENTRY(MACRO_HOLD, code);
				else {
					err("%.*s is not a valid compound key or timeout", (int)key.size(), key.data());
					return -1;
				}
			}

			ADD_ENTRY(MACRO_RELEASE, 0);
			continue;
		} else if (tok.ends_with("ms") && tok.find_first_not_of("0123456789") == tok.size() - 2) {
			ADD_ENTRY(MACRO_TIMEOUT, atoi(tok.data()));
			continue;
		} else {
			uint32_t codepoint;
			if (int chrsz = utf8_read_char(tok, codepoint); chrsz + 0u == tok.size()) {
				if (chrsz == 1 && codepoint < 128) {
					size_t i = 0;
					for (i = 1; i < KEYD_ENTRY_COUNT; i++) {
						const auto name = keycode_table[i].name();
						const char *shiftname = keycode_table[i].shifted_name;

						if (name.size() == 1 && name[0] == tok[0]) {
							ADD_ENTRY(MACRO_KEY_TAP, i).mods = {};
							break;
						}

						if (shiftname && shiftname[0] == tok[0] && shiftname[1] == 0) {
							ADD_ENTRY(MACRO_KEY_TAP, i).mods = { .mods = (1 << MOD_SHIFT), .wildc = 0 };
							break;
						}
					}
					if (i < KEYD_ENTRY_COUNT) {
						continue;
					}
				} else {
					ADD_ENTRY(MACRO_UNICODE, codepoint).id = codepoint >> 16;
					continue;
				}
			}
		}

		err("%.*s is not a valid key sequence", (int)tok.size(), tok.data());
		return -1;
	}
	if (entries.empty()) {
		err("empty macro");
		return -1;
	}

	if (entries.size() == 1) {
		macro.entry = entries[0];
		macro.size = 1;
	} else {
		macro.size = entries.size();
		macro.entries = std::make_unique_for_overwrite<macro_entry[]>(entries.size());
		memcpy(macro.entries.get(), entries.data(), sizeof(macro_entry) * entries.size());
	}

	return 0;

	#undef ADD_ENTRY
}

void macro_execute(void (*output)(uint16_t, uint8_t),
		   const macro& macro, size_t timeout, struct config* config)
{
	int hold_start = -1;

	for (size_t i = 0; i < macro.size; i++) {
		const macro_entry *ent = &macro[i];

		switch (ent->type) {
			size_t j;
			uint32_t idx;
			uint16_t code;
			uint8_t mods;

		case MACRO_HOLD:
			if (hold_start == -1)
				hold_start = i;

			output(ent->id, 1);

			break;
		case MACRO_RELEASE:
			if (hold_start != -1) {
				size_t j;

				for (j = hold_start; j < i; j++) {
					const struct macro_entry *ent = &macro[j];
					output(ent->id, 0);
				}

				hold_start = -1;
			}
			break;
		case MACRO_UNICODE:
			idx = ent->code | (ent->id << 16);

			output(KEY_LEFTCTRL, 1);
			output(KEY_LEFTSHIFT, 1);
			output(KEY_U, 1);
			output(KEY_U, 0);
			output(KEY_LEFTSHIFT, 0);
			output(KEY_LEFTCTRL, 0);

			for (int i = 7 - std::countl_zero(idx) / 4; i >= 0; i--) {
				uint8_t val = (idx >> (i * 4)) % 16;
				output(keys_hex[val], 1);
				output(keys_hex[val], 0);
			}

			output(KEY_ENTER, 1);
			output(KEY_ENTER, 0);
			usleep(10'000);

			break;
		case MACRO_KEY_SEQ:
		case MACRO_KEY_TAP:
			code = ent->id;
			mods = ent->mods.mods;

			for (j = 0; j < config->modifiers.size(); j++) {
				uint16_t code = config->modifiers[j][0];
				uint8_t mask = 1 << j;

				if (mods & mask)
					output(code, 1);
			}

			if (mods && timeout)
				usleep(timeout);

			output(code, 1);
			output(code, 0);

			for (j = 0; j < config->modifiers.size(); j++) {
				uint16_t code = config->modifiers[j][0];
				uint8_t mask = 1 << j;

				if (mods & mask)
					output(code, 0);
			}


			break;
		case MACRO_TIMEOUT:
			usleep(ent->code * 1000);
			break;
		case MACRO_COMMAND:
			extern void execute_command(ucmd& cmd);
			if (config)
				execute_command(config->commands.at(ent->code));
			break;
		default:
			continue;
		}

		if (timeout)
			usleep(timeout);
	}
}
