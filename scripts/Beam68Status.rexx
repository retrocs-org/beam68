/* Beam68 0.1 - launch diagnostics, optional ARexx integration. */
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
'PLUGINS'
SAY 'Plugins:' RESULT
DO i=1 TO 16
  'PLUGIN' i
  IF RC ~= 0 THEN LEAVE
  SAY RESULT
END
EXIT 0
