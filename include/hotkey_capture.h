#ifndef BEAM_HOTKEY_CAPTURE_H
#define BEAM_HOTKEY_CAPTURE_H

#define BEAM_HOTKEY_SHIFT    0x01U
#define BEAM_HOTKEY_CONTROL  0x02U
#define BEAM_HOTKEY_ALT      0x04U
#define BEAM_HOTKEY_LCOMMAND 0x08U
#define BEAM_HOTKEY_RCOMMAND 0x10U

/* Format a Commodities input-description string from one key-down event.
 * mapped_char is the unqualified character returned by keymap.library, or
 * zero for non-character keys. Returns non-zero only for a supported key. */
int beam_hotkey_format(char *out, unsigned capacity, unsigned raw_code,
                       unsigned modifiers, int mapped_char);
int beam_hotkey_is_modifier(unsigned raw_code);
int beam_hotkey_has_duplicates(const char *descriptor);

#endif
