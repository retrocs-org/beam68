/* HelloRexx - a Beam68 plugin written in ARexx (protocol 1).
 *
 * Beam68 runs:  RX HelloRexx.rexx <INPUT-AS-HEX> <reply-file>
 * Type ";Amiga" or "hirexx:Amiga" in the launcher and press Enter.
 * RexxMast must be running (it is started by the standard User-Startup). */
PARSE ARG hex reply .
IF reply = '' THEN EXIT 20

/* The input is hex-encoded so it can never break the command line;
   "-" means an empty input. */
IF hex = '-' THEN input = ''
ELSE input = X2C(hex)

IF input = '' THEN CALL Answer 'ERROR=Type a name, e.g. hirexx:Amiga'
ELSE CALL Answer 'TEXT=Hello from ARexx,' STRIP(input) || '!'
EXIT 0

/* Write exactly one line, atomically: to <reply>.part, then rename, so
   the launcher never reads half a reply.  Keep it under 120 characters. */
Answer: PROCEDURE EXPOSE reply
    PARSE ARG line
    part = reply || '.part'
    IF ~OPEN('out', part, 'W') THEN EXIT 20
    CALL WRITELN('out', LEFT(line, 125))
    CALL CLOSE('out')
    ADDRESS COMMAND 'Rename "' || part || '" "' || reply || '"'
    RETURN
