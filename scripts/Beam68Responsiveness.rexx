/* Beam68 0.4 - capture responsiveness on the current machine and index. */
OPTIONS RESULTS
PARSE ARG report
report=STRIP(report)
IF report='' THEN report='T:Beam68-responsiveness.log'
IF ~SHOW('P','BEAM68') THEN DO
  SAY 'Start Beam68 0.4 with RexxMast available, then run this script again.'
  EXIT 10
END
IF OPEN('existing',report,'R') THEN DO
  CALL CLOSE 'existing'
  SAY 'Report already exists; choose another filename:' report
  EXIT 10
END
IF ~OPEN('log',report,'W') THEN DO
  SAY 'Cannot create report:' report
  EXIT 10
END
ADDRESS 'BEAM68' 'VERSION'
IF RC~=0 | RESULT~='0.4' THEN DO
  SAY 'This report requires Beam68 0.4.'
  CALL CLOSE 'log'
  EXIT 10
END
CALL LINEOUT 'log','Beam68 responsiveness report - version 0.4'
CALL LINEOUT 'log','Captured:' DATE('S') TIME()
SAY 'Machine/CPU, memory, display mode and index label (small/large):'
PULL machine
CALL LINEOUT 'log','Environment:' machine
CALL snapshot 'Before test'
ADDRESS 'BEAM68' 'TIMING RESET'
IF RC~=0 THEN SIGNAL failed
ADDRESS 'BEAM68' 'SHOW'
IF RC~=0 THEN SIGNAL failed
SAY 'In Beam68 type m, mu, then multiview. Pause briefly after each prefix.'
SAY 'Do not press Enter in Beam68. Return here and press Return.'
PULL ready
CALL snapshot 'Growing query'
SAY 'In Beam68 clear the query, then type multiview quickly as one burst.'
SAY 'Do not press Enter in Beam68. Return here and press Return.'
PULL ready
CALL snapshot 'Typing burst'
SAY 'In Beam68 clear the query, type zzzunlikelymatch, then backspace it.'
SAY 'Do not press Enter in Beam68. Return here and press Return.'
PULL ready
CALL snapshot 'No match and backspace'
CALL CLOSE 'log'
SAY 'Saved:' report
SAY 'Repeat with the same workload and a separate filename for each index/run.'
EXIT 0

snapshot: PROCEDURE
  PARSE ARG label
  CALL LINEOUT 'log',label
  ADDRESS 'BEAM68' 'STATUS'
  IF RC~=0 THEN SIGNAL snapshot_failed
  CALL LINEOUT 'log','Status:' RESULT
  ADDRESS 'BEAM68' 'TIMING'
  IF RC~=0 THEN SIGNAL snapshot_failed
  CALL LINEOUT 'log','Timing:' RESULT
  SAY label':' RESULT
  RETURN
snapshot_failed:
  CALL LINEOUT 'log','FAILED: Beam68 command refused or port unavailable.'
  CALL CLOSE 'log'
  SAY 'Measurement failed; inspect the partial report.'
  EXIT 10
failed:
CALL LINEOUT 'log','FAILED: Could not reset timing or show the launcher.'
CALL CLOSE 'log'
EXIT 10
