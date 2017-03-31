; OpenMPT Install script for InnoSetup
; Written by Johannes Schultz
; https://openmpt.org/
; https://sagamusix.de/

; This file is provided for creating an install package without the proprietary unmo3.dll (for the SourceForge package).
#define NO_MO3
#define BaseNameAddition "_sf"
#include "win32.iss"
