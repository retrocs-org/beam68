# Calculator

Plugin version 0.3. Offline integer arithmetic and decimal unit conversion,
without an FPU or network access. Type `=12*(3+4)` or `=$FF+%1010`.

Conversions use `=number source to target` (or `in`):

- `=1.5 MiB to KiB` -> `1536 KiB`
- `=90 min to h` -> `1.5 h`
- `=1 in to cm` -> `2.54 cm`
- `=2 kg to g` -> `2000 g`

Units: B/KB/MB/GB (decimal), KiB/MiB/GiB (binary); ms/s/min/h/day;
mm/cm/m/km/in/ft/yd/mi; mg/g/kg. Units are case insensitive; bits and
currency are not supported. Temperature uses C/F/K, for example `=32 F to C`
and `=0 C to K`. Values below absolute zero are rejected. Input allows three fractional decimal digits.
Output has up to twelve fractional digits; `~` marks truncation of a
non-exact result. Different dimensions, unknown units, trailing text and
32-bit intermediate overflow are rejected. Arithmetic remains integer-only.
