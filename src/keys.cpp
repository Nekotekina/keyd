/*
 * keyd - A key remapping daemon.
 *
 * © 2019 Raheman Vaiya (see also: LICENSE).
 */
#include <stdint.h>
#include <string.h>
#include "keys.h"
#include <array>

const struct modifier modifiers[MAX_MOD] = {
	{MOD_ALT, KEYD_LEFTALT},
	{MOD_ALT_GR, KEYD_RIGHTALT},
	{MOD_SHIFT, KEYD_LEFTSHIFT},
	{MOD_SUPER, KEYD_LEFTMETA},
	{MOD_CTRL, KEYD_LEFTCTRL},
};

extern constexpr std::array<keycode_table_ent, 256> keycode_table = []() {
	std::array<keycode_table_ent, 256> r{};
	r[KEYD_ESC] = { "esc", "escape", NULL },
	r[KEYD_1] = { "1", NULL, "!" },
	r[KEYD_2] = { "2", NULL, "@" },
	r[KEYD_3] = { "3", NULL, "#" },
	r[KEYD_4] = { "4", NULL, "$" },
	r[KEYD_5] = { "5", NULL, "%" },
	r[KEYD_6] = { "6", NULL, "^" },
	r[KEYD_7] = { "7", NULL, "&" },
	r[KEYD_8] = { "8", NULL, "*" },
	r[KEYD_9] = { "9", NULL, "(" },
	r[KEYD_0] = { "0", NULL, ")" },
	r[KEYD_MINUS] = { "-", "minus", "_" },
	r[KEYD_EQUAL] = { "=", "equal", "+" },
	r[KEYD_BACKSPACE] = { "backspace", "\b", NULL },
	r[KEYD_TAB] = { "tab", "\t", NULL },
	r[KEYD_Q] = { "q", NULL, "Q" },
	r[KEYD_W] = { "w", NULL, "W" },
	r[KEYD_E] = { "e", NULL, "E" },
	r[KEYD_R] = { "r", NULL, "R" },
	r[KEYD_T] = { "t", NULL, "T" },
	r[KEYD_Y] = { "y", NULL, "Y" },
	r[KEYD_U] = { "u", NULL, "U" },
	r[KEYD_I] = { "i", NULL, "I" },
	r[KEYD_O] = { "o", NULL, "O" },
	r[KEYD_P] = { "p", NULL, "P" },
	r[KEYD_LEFTBRACE] = { "[", "leftbrace", "{" },
	r[KEYD_RIGHTBRACE] = { "]", "rightbrace", "}" },
	r[KEYD_ENTER] = { "enter", "\n", NULL },
	r[KEYD_LEFTCTRL] = { "leftcontrol", "leftctrl", NULL },
	r[KEYD_IS_LEVEL3_SHIFT] = { "iso-level3-shift", NULL, NULL }, //Oddly missing from input-event-codes.h, appears to be used as altgr in an english keymap on X
	r[KEYD_A] = { "a", NULL, "A" },
	r[KEYD_S] = { "s", NULL, "S" },
	r[KEYD_D] = { "d", NULL, "D" },
	r[KEYD_F] = { "f", NULL, "F" },
	r[KEYD_G] = { "g", NULL, "G" },
	r[KEYD_H] = { "h", NULL, "H" },
	r[KEYD_J] = { "j", NULL, "J" },
	r[KEYD_K] = { "k", NULL, "K" },
	r[KEYD_L] = { "l", NULL, "L" },
	r[KEYD_SEMICOLON] = { ";", "semicolon", ":" },
	r[KEYD_APOSTROPHE] = { "'", "apostrophe", "\"" },
	r[KEYD_GRAVE] = { "`", "grave", "~" },
	r[KEYD_LEFTSHIFT] = { "leftshift", "", NULL },
	r[KEYD_BACKSLASH] = { "\\", "backslash", "|" },
	r[KEYD_Z] = { "z", NULL, "Z" },
	r[KEYD_X] = { "x", NULL, "X" },
	r[KEYD_C] = { "c", NULL, "C" },
	r[KEYD_V] = { "v", NULL, "V" },
	r[KEYD_B] = { "b", NULL, "B" },
	r[KEYD_N] = { "n", NULL, "N" },
	r[KEYD_M] = { "m", NULL, "M" },
	r[KEYD_COMMA] = { ",", "comma", "<" },
	r[KEYD_DOT] = { ".", "dot", ">" },
	r[KEYD_SLASH] = { "/", "slash", "?" },
	r[KEYD_RIGHTSHIFT] = { "rightshift", NULL, NULL },
	r[KEYD_KPASTERISK] = { "kpasterisk", NULL, NULL },
	r[KEYD_LEFTALT] = { "leftalt", "", NULL },
	r[KEYD_SPACE] = { "space", " ", NULL },
	r[KEYD_CAPSLOCK] = { "capslock", NULL, NULL },
	r[KEYD_F1] = { "f1", NULL, NULL },
	r[KEYD_F2] = { "f2", NULL, NULL },
	r[KEYD_F3] = { "f3", NULL, NULL },
	r[KEYD_F4] = { "f4", NULL, NULL },
	r[KEYD_F5] = { "f5", NULL, NULL },
	r[KEYD_F6] = { "f6", NULL, NULL },
	r[KEYD_F7] = { "f7", NULL, NULL },
	r[KEYD_F8] = { "f8", NULL, NULL },
	r[KEYD_F9] = { "f9", NULL, NULL },
	r[KEYD_F10] = { "f10", NULL, NULL },
	r[KEYD_NUMLOCK] = { "numlock", NULL, NULL },
	r[KEYD_SCROLLLOCK] = { "scrolllock", NULL, NULL },
	r[KEYD_KP7] = { "kp7", NULL, NULL },
	r[KEYD_KP8] = { "kp8", NULL, NULL },
	r[KEYD_KP9] = { "kp9", NULL, NULL },
	r[KEYD_KPMINUS] = { "kpminus", NULL, NULL },
	r[KEYD_KP4] = { "kp4", NULL, NULL },
	r[KEYD_KP5] = { "kp5", NULL, NULL },
	r[KEYD_KP6] = { "kp6", NULL, NULL },
	r[KEYD_KPPLUS] = { "kpplus", NULL, NULL },
	r[KEYD_KP1] = { "kp1", NULL, NULL },
	r[KEYD_KP2] = { "kp2", NULL, NULL },
	r[KEYD_KP3] = { "kp3", NULL, NULL },
	r[KEYD_KP0] = { "kp0", NULL, NULL },
	r[KEYD_KPDOT] = { "kpdot", NULL, NULL },
	r[KEYD_ZENKAKUHANKAKU] = { "zenkakuhankaku", NULL, NULL },
	r[KEYD_102ND] = { "102nd", NULL, NULL },
	r[KEYD_F11] = { "f11", NULL, NULL },
	r[KEYD_F12] = { "f12", NULL, NULL },
	r[KEYD_RO] = { "ro", NULL, NULL },
	r[KEYD_KATAKANA] = { "katakana", NULL, NULL },
	r[KEYD_HIRAGANA] = { "hiragana", NULL, NULL },
	r[KEYD_HENKAN] = { "henkan", NULL, NULL },
	r[KEYD_KATAKANAHIRAGANA] = { "katakanahiragana", NULL, NULL },
	r[KEYD_MUHENKAN] = { "muhenkan", NULL, NULL },
	r[KEYD_KPJPCOMMA] = { "kpjpcomma", NULL, NULL },
	r[KEYD_KPENTER] = { "kpenter", NULL, NULL },
	r[KEYD_RIGHTCTRL] = { "rightcontrol", "rightctrl", NULL },
	r[KEYD_KPSLASH] = { "kpslash", NULL, NULL },
	r[KEYD_SYSRQ] = { "sysrq", NULL, NULL },
	r[KEYD_RIGHTALT] = { "rightalt", NULL, NULL },
	r[KEYD_LINEFEED] = { "linefeed", NULL, NULL },
	r[KEYD_HOME] = { "home", NULL, NULL },
	r[KEYD_UP] = { "up", NULL, NULL },
	r[KEYD_PAGEUP] = { "pageup", NULL, NULL },
	r[KEYD_LEFT] = { "left", NULL, NULL },
	r[KEYD_RIGHT] = { "right", NULL, NULL },
	r[KEYD_END] = { "end", NULL, NULL },
	r[KEYD_DOWN] = { "down", NULL, NULL },
	r[KEYD_PAGEDOWN] = { "pagedown", NULL, NULL },
	r[KEYD_INSERT] = { "insert", NULL, NULL },
	r[KEYD_DELETE] = { "delete", NULL, NULL },
	r[KEYD_MACRO] = { "macro", NULL, NULL },
	r[KEYD_MUTE] = { "mute", NULL, NULL },
	r[KEYD_VOLUMEDOWN] = { "volumedown", NULL, NULL },
	r[KEYD_VOLUMEUP] = { "volumeup", NULL, NULL },
	r[KEYD_POWER] = { "power", NULL, NULL },
	r[KEYD_KPEQUAL] = { "kpequal", NULL, NULL },
	r[KEYD_KPPLUSMINUS] = { "kpplusminus", NULL, NULL },
	r[KEYD_PAUSE] = { "pause", NULL, NULL },
	r[KEYD_SCALE] = { "scale", NULL, NULL },
	r[KEYD_KPCOMMA] = { "kpcomma", NULL, NULL },
	r[KEYD_HANGEUL] = { "hangeul", NULL, NULL },
	r[KEYD_HANJA] = { "hanja", NULL, NULL },
	r[KEYD_YEN] = { "yen", NULL, NULL },
	r[KEYD_LEFTMETA] = { "leftmeta", "leftsuper", NULL },
	r[KEYD_RIGHTMETA] = { "rightmeta", "rightsuper", NULL },
	r[KEYD_COMPOSE] = { "compose", NULL, NULL },
	r[KEYD_STOP] = { "stop", NULL, NULL },
	r[KEYD_AGAIN] = { "again", NULL, NULL },
	r[KEYD_PROPS] = { "props", NULL, NULL },
	r[KEYD_UNDO] = { "undo", NULL, NULL },
	r[KEYD_FRONT] = { "front", NULL, NULL },
	r[KEYD_COPY] = { "copy", NULL, NULL },
	r[KEYD_OPEN] = { "open", NULL, NULL },
	r[KEYD_PASTE] = { "paste", NULL, NULL },
	r[KEYD_FIND] = { "find", NULL, NULL },
	r[KEYD_CUT] = { "cut", NULL, NULL },
	r[KEYD_HELP] = { "help", NULL, NULL },
	r[KEYD_MENU] = { "menu", NULL, NULL },
	r[KEYD_CALC] = { "calc", NULL, NULL },
	r[KEYD_SETUP] = { "setup", NULL, NULL },
	r[KEYD_SLEEP] = { "sleep", NULL, NULL },
	r[KEYD_WAKEUP] = { "wakeup", NULL, NULL },
	r[KEYD_FILE] = { "file", NULL, NULL },
	r[KEYD_SENDFILE] = { "sendfile", NULL, NULL },
	r[KEYD_DELETEFILE] = { "deletefile", NULL, NULL },
	r[KEYD_XFER] = { "xfer", NULL, NULL },
	r[KEYD_PROG1] = { "prog1", NULL, NULL },
	r[KEYD_PROG2] = { "prog2", NULL, NULL },
	r[KEYD_WWW] = { "www", NULL, NULL },
	r[KEYD_MSDOS] = { "msdos", NULL, NULL },
	r[KEYD_COFFEE] = { "coffee", NULL, NULL },
	r[KEYD_ROTATE_DISPLAY] = { "display", NULL, NULL },
	r[KEYD_CYCLEWINDOWS] = { "cyclewindows", NULL, NULL },
	r[KEYD_MAIL] = { "mail", NULL, NULL },
	r[KEYD_BOOKMARKS] = { "bookmarks", NULL, NULL },
	r[KEYD_COMPUTER] = { "computer", NULL, NULL },
	r[KEYD_BACK] = { "back", NULL, NULL },
	r[KEYD_FORWARD] = { "forward", NULL, NULL },
	r[KEYD_CLOSECD] = { "closecd", NULL, NULL },
	r[KEYD_EJECTCD] = { "ejectcd", NULL, NULL },
	r[KEYD_EJECTCLOSECD] = { "ejectclosecd", NULL, NULL },
	r[KEYD_NEXTSONG] = { "nextsong", NULL, NULL },
	r[KEYD_PLAYPAUSE] = { "playpause", NULL, NULL },
	r[KEYD_PREVIOUSSONG] = { "previoussong", NULL, NULL },
	r[KEYD_STOPCD] = { "stopcd", NULL, NULL },
	r[KEYD_RECORD] = { "record", NULL, NULL },
	r[KEYD_REWIND] = { "rewind", NULL, NULL },
	r[KEYD_PHONE] = { "phone", NULL, NULL },
	r[KEYD_ISO] = { "iso", NULL, NULL },
	r[KEYD_CONFIG] = { "config", NULL, NULL },
	r[KEYD_HOMEPAGE] = { "homepage", NULL, NULL },
	r[KEYD_REFRESH] = { "refresh", NULL, NULL },
	r[KEYD_EXIT] = { "exit", NULL, NULL },
	r[KEYD_MOVE] = { "move", NULL, NULL },
	r[KEYD_EDIT] = { "edit", NULL, NULL },
	r[KEYD_KPLEFTPAREN] = { "kpleftparen", NULL, NULL },
	r[KEYD_KPRIGHTPAREN] = { "kprightparen", NULL, NULL },
	r[KEYD_NEW] = { "new", NULL, NULL },
	r[KEYD_REDO] = { "redo", NULL, NULL },
	r[KEYD_F13] = { "f13", NULL, NULL },
	r[KEYD_F14] = { "f14", NULL, NULL },
	r[KEYD_F15] = { "f15", NULL, NULL },
	r[KEYD_F16] = { "f16", NULL, NULL },
	r[KEYD_F17] = { "f17", NULL, NULL },
	r[KEYD_F18] = { "f18", NULL, NULL },
	r[KEYD_F19] = { "f19", NULL, NULL },
	r[KEYD_F20] = { "f20", NULL, NULL },
	r[KEYD_F21] = { "f21", NULL, NULL },
	r[KEYD_F22] = { "f22", NULL, NULL },
	r[KEYD_F23] = { "f23", NULL, NULL },
	r[KEYD_F24] = { "f24", NULL, NULL },
	r[KEYD_PLAYCD] = { "playcd", NULL, NULL },
	r[KEYD_PAUSECD] = { "pausecd", NULL, NULL },
	r[KEYD_PROG3] = { "prog3", NULL, NULL },
	r[KEYD_PROG4] = { "prog4", NULL, NULL },
	r[KEYD_DASHBOARD] = { "dashboard", NULL, NULL },
	r[KEYD_SUSPEND] = { "suspend", NULL, NULL },
	r[KEYD_CLOSE] = { "close", NULL, NULL },
	r[KEYD_PLAY] = { "play", NULL, NULL },
	r[KEYD_FASTFORWARD] = { "fastforward", NULL, NULL },
	r[KEYD_BASSBOOST] = { "bassboost", NULL, NULL },
	r[KEYD_PRINT] = { "print", NULL, NULL },
	r[KEYD_HP] = { "hp", NULL, NULL },
	r[KEYD_CAMERA] = { "camera", NULL, NULL },
	r[KEYD_SOUND] = { "sound", NULL, NULL },
	r[KEYD_QUESTION] = { "question", NULL, NULL },
	r[KEYD_EMAIL] = { "email", NULL, NULL },
	r[KEYD_CHAT] = { "chat", NULL, NULL },
	r[KEYD_SEARCH] = { "search", NULL, NULL },
	r[KEYD_CONNECT] = { "connect", NULL, NULL },
	r[KEYD_FINANCE] = { "finance", NULL, NULL },
	r[KEYD_SPORT] = { "sport", NULL, NULL },
	r[KEYD_SHOP] = { "shop", NULL, NULL },
	r[KEYD_VOICECOMMAND] = { "voicecommand", NULL, NULL },
	r[KEYD_CANCEL] = { "cancel", NULL, NULL },
	r[KEYD_BRIGHTNESSDOWN] = { "brightnessdown", NULL, NULL },
	r[KEYD_BRIGHTNESSUP] = { "brightnessup", NULL, NULL },
	r[KEYD_MEDIA] = { "media", NULL, NULL },
	r[KEYD_SWITCHVIDEOMODE] = { "switchvideomode", NULL, NULL },
	r[KEYD_KBDILLUMTOGGLE] = { "kbdillumtoggle", NULL, NULL },
	r[KEYD_KBDILLUMDOWN] = { "kbdillumdown", NULL, NULL },
	r[KEYD_KBDILLUMUP] = { "kbdillumup", NULL, NULL },
	r[KEYD_SEND] = { "send", NULL, NULL },
	r[KEYD_REPLY] = { "reply", NULL, NULL },
	r[KEYD_FORWARDMAIL] = { "forwardmail", NULL, NULL },
	r[KEYD_SAVE] = { "save", NULL, NULL },
	r[KEYD_DOCUMENTS] = { "documents", NULL, NULL },
	r[KEYD_BATTERY] = { "battery", NULL, NULL },
	r[KEYD_BLUETOOTH] = { "bluetooth", NULL, NULL },
	r[KEYD_WLAN] = { "wlan", NULL, NULL },
	r[KEYD_UWB] = { "uwb", NULL, NULL },
	r[KEYD_UNKNOWN] = { "unknown", NULL, NULL },
	r[KEYD_VIDEO_NEXT] = { "next", NULL, NULL },
	r[KEYD_VIDEO_PREV] = { "prev", NULL, NULL },
	r[KEYD_BRIGHTNESS_CYCLE] = { "cycle", NULL, NULL },
	r[KEYD_BRIGHTNESS_AUTO] = { "auto", NULL, NULL },
	r[KEYD_DISPLAY_OFF] = { "off", NULL, NULL },
	r[KEYD_WWAN] = { "wwan", NULL, NULL },
	r[KEYD_RFKILL] = { "rfkill", NULL, NULL },
	r[KEYD_MICMUTE] = { "micmute", NULL, NULL },
	r[KEYD_LEFT_MOUSE] = { "leftmouse", NULL, NULL },
	r[KEYD_RIGHT_MOUSE] = { "rightmouse", NULL, NULL },
	r[KEYD_MIDDLE_MOUSE] = { "middlemouse", NULL, NULL },
	r[KEYD_MOUSE_1] = { "mouse1", NULL, NULL },
	r[KEYD_MOUSE_2] = { "mouse2", NULL, NULL },
	r[KEYD_MOUSE_BACK] = { "mouseback", NULL, NULL },
	r[KEYD_MOUSE_FORWARD] = { "mouseforward", NULL, NULL },
	r[KEYD_FN] = { "fn", NULL, NULL },
	r[KEYD_ZOOM] = { "zoom", NULL, NULL },
	r[KEYD_NOOP] = { "noop", NULL, NULL },
	void();
	return r;
}();

const char *modstring(uint8_t mods)
{
	static char s[16];
	int i = 0;
	s[0] = 0;

	if (MOD_CTRL & mods) {
		s[i++] = 'C';
		s[i++] = '-';
	}

	if (MOD_SUPER & mods) {
		s[i++] = 'M';
		s[i++] = '-';
	}

	if (MOD_ALT_GR & mods) {
		s[i++] = 'G';
		s[i++] = '-';
	}

	if (MOD_SHIFT & mods) {
		s[i++] = 'S';
		s[i++] = '-';
	}

	if (MOD_ALT & mods) {
		s[i++] = 'A';
		s[i++] = '-';
	}

	if(i)
		s[i-1] = 0;

	return s;
}

int parse_modset(const char *s, uint8_t *mods)
{
	*mods = 0;

	while (*s) {
		switch (*s) {
		case 'C':
			*mods |= MOD_CTRL;
			break;
		case 'M':
			*mods |= MOD_SUPER;
			break;
		case 'A':
			*mods |= MOD_ALT;
			break;
		case 'S':
			*mods |= MOD_SHIFT;
			break;
		case 'G':
			*mods |= MOD_ALT_GR;
			break;
		default:
			return -1;
			break;
		}

		if (s[1] == 0)
			return 0;
		else if (s[1] != '-')
			return -1;

		s += 2;
	}

	return 0;
}

int parse_key_sequence(std::string_view s, uint8_t *codep, uint8_t *modsp)
{
	auto c = s;
	if (s.empty())
		return -1;

	uint8_t mods = 0;

	while (c.size() >= 2 && c[1] == '-') {
		switch (c[0]) {
		case 'C':
			mods |= MOD_CTRL;
			break;
		case 'M':
			mods |= MOD_SUPER;
			break;
		case 'A':
			mods |= MOD_ALT;
			break;
		case 'S':
			mods |= MOD_SHIFT;
			break;
		case 'G':
			mods |= MOD_ALT_GR;
			break;
		default:
			return -1;
			break;
		}

		c.remove_prefix(2);
	}

	for (size_t i = 0; i < 256; i++) {
		const struct keycode_table_ent *ent = &keycode_table[i];

		if (ent->name) {
			if (ent->shifted_name && ent->shifted_name == c) {

				mods |= MOD_SHIFT;

				if (modsp)
					*modsp = mods;

				if (codep)
					*codep = i;

				return 0;
			} else if (ent->name == c || (ent->alt_name && ent->alt_name == c)) {

				if (modsp)
					*modsp = mods;

				if (codep)
					*codep = i;

				return 0;
			}
		}
	}

	return -1;
}
