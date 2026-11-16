/* Beam68 0.3 - launch diagnostics, optional ARexx integration. */
OPTIONS RESULTS
IF ~SHOW('P','BEAM68') THEN DO
  SAY 'The Beam68 ARexx port is not available.'
  EXIT 10
END
ADDRESS 'BEAM68'
'STATUS'
SAY 'Beam68:' RESULT
'INDEX'
SAY 'Index:' RESULT
'TIMING'
SAY 'Timing:' RESULT
'REQUESTS'
IF RC=0 THEN DO
  SAY 'Requests:' RESULT
  DO slot=1 TO 3
    'REQUEST' slot
    IF RC~=0 THEN LEAVE
    SAY RESULT
  END
END
'PLUGINS'
SAY 'Plugins:' RESULT
DO i=1 TO 32
  'PLUGIN' i
  IF RC ~= 0 THEN LEAVE
  SAY RESULT
END
EXIT 0
