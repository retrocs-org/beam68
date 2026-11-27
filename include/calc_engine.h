#ifndef BEAM_CALC_ENGINE_H
#define BEAM_CALC_ENGINE_H
/* 32-bit signed integer arithmetic; no FPU required. */
/* Literals: decimal 255, hexadecimal $FF or 0xFF, binary %11111111. */
int beam_calculate(const char *expression, long *answer);
/* "255 $FF %11111111" (binary only for 0..65535). */
void beam_calc_format(long value, char *out, unsigned int cap);
/* Arithmetic or offline decimal units: "1.5 MiB to KiB". Three input decimals; up to twelve output decimals; C/F/K temperatures;
 * ~ marks a truncated non-exact result. Failure leaves output empty. */
int beam_calc_result(const char *input,char *out,unsigned int capacity);
#endif
