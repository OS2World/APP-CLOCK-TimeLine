#-------------------
# TIMELINE make file
#-------------------

timeline.obj : timeline.c
  cl -c -G2sw -W3 -AL -Zi -Od timeline.c
  
timeline.exe : timeline.obj timeline.def
  link /CO timeline, /align:16, NUL, os2, timeline
  rc timeline.rc
  touch timeline.exe

timeline.exe : timeline.res
  rc timeline.rc
