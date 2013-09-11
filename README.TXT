As my first OS/2 Presentation Manager program, I extensively commented and
then extended Charles Petzold's TIMELINE.C program from Volume 8 Number 6
(March 28, 1989) of "PC Magazine."  For acknowledgements, restrictions,
and instructions on what this version of TIMELINE does, see the front of
TIMELINE.C.  I hope someone can find it as useful as I have.

TIMELINE.C was compiled with Microsoft C 5.1 and the header files and tools
from the Microsoft OS/2 SDK 1.06 (the equivalent of the IBM OS/2 SE 1.1
files).

Oddities:

1. The name of the make file is TIMELINE.MAK rather than the conventional
   TIMELINE.

2. I have compiled and linked TIMELINE as a large model and ready to be used
   with Codeview.

3. TIMELINE.MAK references a program not distributed with TIMELINE, TOUCH
   (which updates the creation time of a file to the current time), to make
   sure that the TIMELINE resource file isn't created more frequently than
   necessary.  You can safely remove the TOUCH line from TIMELINE.MAK if
   if you do not have your own program similar to TOUCH.

Eric Baatz
CIS 74010,3664
April 29, 1989
