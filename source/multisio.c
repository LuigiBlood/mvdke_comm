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

/* Code taken from multi_sio exemple from GBA SDK 3.0 and is converted to work on devkitARM */

static const u32 zero = 0;

static const u8 MultiSioLib_Var[]="MultiSio_DKARM";

void MultiSioInit(void) {	
	irqDisable(IRQ_SERIAL | MULTI_SIO_TIMER_INTR_FLAG);
	
	REG_RCNT = R_MULTI;
	REG_SIOCNT = MULTI_SIO_BAUD_RATE_NO | SIO_MULTI | SIO_IRQ;
	CpuSet(&zero, &Ms, (sizeof(Ms) / 2) | FILL);
	
	Ms.SendBufCounter     = -1;

	Ms.NextSendBufp    = (u16 *)&Ms.SendBuf[0];		// Set Send Buffer Pointer
	Ms.CurrentSendBufp = (u16 *)&Ms.SendBuf[1];

	for (s8 i=0; i<MULTI_SIO_PLAYERS_MAX; i++) {		// Set Receive Buffer Pointer
		Ms.CurrentRecvBufp[i] = (u16 *)&Ms.RecvBuf[i][0];
		Ms.LastRecvBufp[i]    = (u16 *)&Ms.RecvBuf[i][1];
		Ms.RecvCheckBufp[i]   = (u16 *)&Ms.RecvBuf[i][2];
	}
	
	irqSet(IRQ_SERIAL, MultiSioIntr);
	
	irqEnable(IRQ_SERIAL);
}

u32 MultiSioMain(void *Sendp, void *Recvp)
{
	u16	SioCntBak;

	switch (Ms.State) {
		case 0: SioCntBak = REG_SIOCNT;     // Check Connection
			if ((SioCntBak & 0x30) == 0) {
				if (!(SioCntBak & SIO_SO_HIGH) || (SioCntBak & SIO_IRQ))    break;
				if (!(SioCntBak & SIO_RDY) && Ms.SendBufCounter == -1) {
					irqDisable(IRQ_SERIAL);
					irqEnable(MULTI_SIO_TIMER_INTR_FLAG);
					
					REG_SIOCNT &= ~SIO_IRQ;
					
					irqEnable(IRQ_SERIAL | MULTI_SIO_TIMER_INTR_FLAG);

					*(vu32 *)REG_MULTI_SIO_TIMER                    // Timer Initialization
						= MULTI_SIO_TIMER_COUNT;

					Ms.Type = 8;
				}
			}
			Ms.State = 1;
		case 1: MultiSioRecvDataCheck(Recvp);           // Check Receive Data
			MultiSioSendDataSet(Sendp);             // Set Send Data
			break;
	}

	Ms.SendFrameCounter++;

	return	Ms.RecvSuccessFlags
		| (Ms.Type == 8) << 7
		| Ms.ConnectedFlags << 8;
}

void MultiSioSendDataSet(void *Sendp)
{
    s32     CheckSum = MULTI_SIO_CHECKSUM_DIFF;
    int     i;

    ((MultiSioPacket *)Ms.NextSendBufp)->FrameCounter = (u8 )Ms.SendFrameCounter;
    ((MultiSioPacket *)Ms.NextSendBufp)->RecvErrorFlags =  Ms.ConnectedFlags ^ Ms.RecvSuccessFlags;
    ((MultiSioPacket *)Ms.NextSendBufp)->CheckSum = 0;

    CpuSet(Sendp, (u8 *)&Ms.NextSendBufp[2], (MULTI_SIO_BLOCK_SIZE / 2) | COPY16);

    for (i=0; i<sizeof(MultiSioPacket)/2 - 2; i++)      // Calculate Checksum Send data
        CheckSum += Ms.NextSendBufp[i];
    ((MultiSioPacket *)Ms.NextSendBufp)->CheckSum = ~CheckSum;

    if (Ms.Type)
        *(vu16 *)REG_MULTI_SIO_TIMER_H = 0;             // Stop Timer 

    Ms.SendBufCounter = -1;                             // Update Send Data

    if (Ms.Type && Ms.StartFlag)
        *(vu16 *)REG_MULTI_SIO_TIMER_H                  // Start Timer 
                             = (0 | TIMER_IRQ | TIMER_START);
}

u32 MultiSioRecvDataCheck(void *Recvp)
{
    u16     *BufpTmp;
    s32      CheckSum;
    u8       SyncRecvFlagBak[4];
    int      i, ii;

    REG_IME = 0;                               // Disable Interrupts (Approx. 80 Clocks)

    for (i=0; i<4; i++) {
        BufpTmp = Ms.RecvCheckBufp[i];                  // Update Receive Data/Check Buffer
        Ms.RecvCheckBufp[i] = Ms.LastRecvBufp[i];
        Ms.LastRecvBufp[i] = BufpTmp;
    }
    *(u32 *)SyncRecvFlagBak = *(u32 *)Ms.SyncRecvFlag;  // Copy Synchronized Data Receive Confirmation Flag
    *(u32 *)Ms.SyncRecvFlag = 0;

    REG_IME = 1;                               // Enable Interrupt

    Ms.RecvSuccessFlags = 0;

    for (i=0; i<MULTI_SIO_PLAYERS_MAX; i++) {
        CheckSum = MULTI_SIO_CHECKSUM_DIFF;            // Calculate Checksum Receive Data
        for (ii=0; ii<sizeof(MultiSioPacket)/2 - 2; ii++)
            CheckSum +=  Ms.RecvCheckBufp[i][ii];

        if (SyncRecvFlagBak[i])                         // Receive Success Confirmation
            if ((s16 )CheckSum == -1) {
                CpuSet(&((u8 *)Ms.RecvCheckBufp[i])[4],
                        &((u8 *)Recvp)[i*MULTI_SIO_BLOCK_SIZE], (MULTI_SIO_BLOCK_SIZE / 2) | COPY16);
                Ms.RecvSuccessFlags |= 1 << i;
            }

        CpuSet(&zero, &((u8 *)Ms.RecvCheckBufp[i])[4], (MULTI_SIO_BLOCK_SIZE / 2) | FILL);
    }

    Ms.ConnectedFlags |= Ms.RecvSuccessFlags;           // Set Connect End Flag

    return Ms.RecvSuccessFlags;
}

void MultiSioIntr(void)
{
    u16      RecvTmp[4];
    u16     *BufpTmp;
    int     i;


    // Save Receive Data
    RecvTmp[0] = REG_SIOMULTI0;
    RecvTmp[1] = REG_SIOMULTI1;
    RecvTmp[2] = REG_SIOMULTI2;
    RecvTmp[3] = REG_SIOMULTI3;


    // Send Data Processing

    if (Ms.SendBufCounter == -1) {                      // Set Synchronized Data
        REG_SIOMLT_SEND = MULTI_SIO_SYNC_DATA;

        BufpTmp = Ms.CurrentSendBufp;                   // Change Send Buffer
        Ms.CurrentSendBufp = Ms.NextSendBufp;
        Ms.NextSendBufp = BufpTmp;
    } else if (Ms.SendBufCounter >= 0) {                // Set Send Data
        REG_SIOMLT_SEND = Ms.CurrentSendBufp[Ms.SendBufCounter];
    }
    if (Ms.SendBufCounter < (s32 )(sizeof(MultiSioPacket)/2 - 1))  Ms.SendBufCounter++;


    // Receive Data Processing (Max. of Approx. 350 Clocks/Included in wait period)

    for (i=0; i<MULTI_SIO_PLAYERS_MAX; i++) {
        if (RecvTmp[i] == MULTI_SIO_SYNC_DATA
         && Ms.RecvBufCounter[i] > (s32 )(sizeof(MultiSioPacket)/2 - 3)) {
            Ms.RecvBufCounter[i] = -1;
        } else {
            Ms.CurrentRecvBufp[i][Ms.RecvBufCounter[i]] = RecvTmp[i];
                                                        // Store Receive Data
            if (Ms.RecvBufCounter[i] == (s32 )(sizeof(MultiSioPacket)/2 - 3)) {
                BufpTmp = Ms.LastRecvBufp[i];           // Change Receive Buffer
                Ms.LastRecvBufp[i] = Ms.CurrentRecvBufp[i];
                Ms.CurrentRecvBufp[i] = BufpTmp;
                Ms.SyncRecvFlag[i] = 1;                 // Receive End
            }
        }
        if (Ms.RecvBufCounter[i] < (s32 )(sizeof(MultiSioPacket)/2 - 1))  Ms.RecvBufCounter[i]++;
    }


    // Start Master Send

    if (Ms.Type == 8) {
        *(vu16 *)REG_MULTI_SIO_TIMER_H = 0;             // Stop Timer
        REG_SIOCNT |= SIO_START;              // Restart Send
        *(vu16 *)REG_MULTI_SIO_TIMER_H                  // Restart Timer
                             = (0 | TIMER_IRQ | TIMER_START);
    }
}
