
#include <gba_console.h>
#include <gba_video.h>
#include <gba_base.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>
#include <gba_sio.h>
#include <gba_timers.h>
#include <stdio.h>
#include <stdlib.h>
#include "multisio.h"
#include "mvsdke.h"

#include "level_bin.h"

#define	GetCommand()	(UsrRecvBuf[0][0] & 0xFF)
#define	GetPart()		(UsrRecvBuf[0][0] >> 8)
#define BlockSize	(MULTI_SIO_BLOCK_SIZE - 4)

MultiSioArea     Ms;            // Multi-play Communication Work Area

static u16      UsrSendBuf[MULTI_SIO_BLOCK_SIZE/2];    // User Send Buffer
static u16      UsrRecvBuf[MULTI_SIO_PLAYERS_MAX][MULTI_SIO_BLOCK_SIZE/2];
                                                    // User Receive Buffer

void PrepareData(u16 part, u8 data[], u32 size)
{
	if (((part + 1) * BlockSize) <= size)
	{
		UsrSendBuf[0] = (part << 8) | MVDKE_COMM_SLAVE_SEND_PART;
		UsrSendBuf[1] = BlockSize;
	}
	else
	{
		UsrSendBuf[0] = (part << 8) | MVDKE_COMM_SLAVE_SEND_LAST_PART;
		UsrSendBuf[1] = size % BlockSize;
	}
	
	for (int i = 2; i < (MULTI_SIO_BLOCK_SIZE / 2); i++)
	{
		UsrSendBuf[i] = 0;
	}
	
	for (int i = 0; i < UsrSendBuf[1]; i++)
	{
		if (i & 1)
			UsrSendBuf[2 + (i >> 1)] |= data[i + (part * BlockSize)] << 8;
		else
			UsrSendBuf[2 + (i >> 1)] = data[i + (part * BlockSize)];
	}
}

//---------------------------------------------------------------------------------
// Program entry point
//---------------------------------------------------------------------------------
int main(void) {
//---------------------------------------------------------------------------------
	char strText[31] = "";
	u32 StuffFlag = 0;
	
	// the vblank interrupt must be enabled for VBlankIntrWait() to work
	// since the default dispatcher handles the bios flags no vblank handler
	// is required
	irqInit();
	irqEnable(IRQ_VBLANK);
	
	UsrSendBuf[0] = 0x0000;
	
	MultiSioInit();

	consoleDemoInit();
	// ansi escape sequence to set print co-ordinates
	// /x1b[line;columnH
	iprintf("\x1b[10;10HMvsDKTest2\n");

	while (1) {
		iprintf("\x1b[2;2HGBA1\n");
		sprintf(strText, "\x1b[3;2H%04X\n", UsrRecvBuf[0][0]);
		iprintf(strText);
		sprintf(strText, "\x1b[4;2H%04X\n", UsrRecvBuf[0][1]);
		iprintf(strText);
		sprintf(strText, "\x1b[5;2H%04X\n", UsrRecvBuf[0][2]);
		iprintf(strText);
		sprintf(strText, "\x1b[6;2H%04X\n", UsrRecvBuf[0][3]);
		iprintf(strText);
		sprintf(strText, "\x1b[7;2H%04X\n", UsrRecvBuf[0][4]);
		iprintf(strText);
		
		iprintf("\x1b[2;8HGBA2\n");
		sprintf(strText, "\x1b[3;8H%04X\n", UsrRecvBuf[1][0]);
		iprintf(strText);
		sprintf(strText, "\x1b[4;8H%04X\n", UsrRecvBuf[1][1]);
		iprintf(strText);
		sprintf(strText, "\x1b[5;8H%04X\n", UsrRecvBuf[1][2]);
		iprintf(strText);
		sprintf(strText, "\x1b[6;8H%04X\n", UsrRecvBuf[1][3]);
		iprintf(strText);
		sprintf(strText, "\x1b[7;8H%04X\n", UsrRecvBuf[1][4]);
		iprintf(strText);
		
		iprintf("\x1b[2;14HGBA3\n");
		sprintf(strText, "\x1b[3;14H%04X\n", UsrRecvBuf[2][0]);
		iprintf(strText);
		sprintf(strText, "\x1b[4;14H%04X\n", UsrRecvBuf[2][1]);
		iprintf(strText);
		sprintf(strText, "\x1b[5;14H%04X\n", UsrRecvBuf[2][2]);
		iprintf(strText);
		sprintf(strText, "\x1b[6;14H%04X\n", UsrRecvBuf[2][3]);
		iprintf(strText);
		sprintf(strText, "\x1b[7;14H%04X\n", UsrRecvBuf[2][4]);
		iprintf(strText);
		
		
		sprintf(strText, "\x1b[12;12H%04X\n", UsrSendBuf[0]);
		iprintf(strText);
		sprintf(strText, "\x1b[13;13H%04X\n", StuffFlag);
		iprintf(strText);
		VBlankIntrWait();
		
		StuffFlag = MultiSioMain(UsrSendBuf, UsrRecvBuf);
		if (StuffFlag & MULTI_SIO_RECV_ID0) // Multi-play Communication Main
		{
			if (GetCommand() == MVDKE_COMM_MASTER_HANDSHAKE)
			{
				//Available
				UsrSendBuf[0] = MVDKE_COMM_SLAVE_HANDSHAKE;
			}
			else if (GetCommand() == MVDKE_COMM_MASTER_RDY)
			{
				//Ready for transmission
				UsrSendBuf[0] = MVDKE_COMM_SLAVE_DATA_RDY;
			}
			else if (GetCommand() == MVDKE_COMM_MASTER_REQ_PART)
			{
				//Send Data
				PrepareData(GetPart(), level_bin, level_bin_size);
			}
		}
	}
}


