#ifndef MODCOMMAND_H
#define MODCOMMAND_H

typedef struct _MODCOMMAND
{
	BYTE note;
	BYTE instr;
	BYTE volcmd;
	BYTE command;
	BYTE vol;
	BYTE param;
} MODCOMMAND, *LPMODCOMMAND;

#endif
