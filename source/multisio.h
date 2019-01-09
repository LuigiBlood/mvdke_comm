#ifndef _MULTISIO_H_
#define _MULTISIO_H_

// Optimize the following settings based on the software specifications

#define MULTI_SIO_BLOCK_SIZE        16      // Communication Data Block Size (Max. 24 Bytes)

#define MULTI_SIO_PLAYERS_MAX       2       // Maximum Number of Players (1 to 4)

#define MULTI_SIO_SYNC_DATA         0xfefe  // Synchronized Data (0x0000/0xfffa~0xffff prohibited)

#define MULTI_SIO_CHECKSUM_DIFF     0xc     // Some games changes the checksum calculation by adding a constant
                                            // Mario VS Donkey Kong e-Reader Communication = 0xc
                                            // Super Mario Advance 4 e-Reader Communication = ?

// Comment out if no space in CPU internal Work RAM
#define MULTI_SIO_DI_FUNC_FAST              // SIO Interrupt Prohibit Function High Speed Flag (CPU Internal RAM Execution)


// Update if maximum delay for communication interrupt is larger than following.
#define MULTI_SIO_INTR_DELAY_MAX    2000    // Communication Interrupt Allowed Delay Clocks

#ifdef  MULTI_SIO_DI_FUNC_FAST
#define MULTI_SIO_INTR_CLOCK_MAX    400     // Communication Interrupt Processing Maximum Clocks
#else
#define MULTI_SIO_INTR_CLOCK_MAX    1000
#endif

#define MULTI_SIO_1P_SEND_CLOCKS    3000    // Communication Time for 1 unit

#if     MULTI_SIO_PLAYERS_MAX == 4
#define MULTI_SIO_START_BIT_WAIT    0       // Start Bit Wait Time
#else
#define MULTI_SIO_START_BIT_WAIT    512
#endif

// During development set NDEBUG to undefined and value below to 0,
// define with last check and confirm operation with changed to 600.
// (Even if increase setting the communication interval increases, but 
//  processing doesn't slow).
//#define NDEBUG                              // Can define with Makefile (MakefileDemo)
#ifdef  NDEBUG
#define MULTI_SIO_INTR_MARGIN       600     // Communication Interrupt Error Guarantee Value
#else
#define MULTI_SIO_INTR_MARGIN       0
#endif


#define MULTI_SIO_BAUD_RATE         115200          // Baud Rate
#define MULTI_SIO_BAUD_RATE_NO      SIO_115200      // Baud Rate No.


#define MULTI_SIO_TIMER_NO          3       // Timer No.
#define MULTI_SIO_TIMER_INTR_FLAG   (IRQ_TIMER0 << MULTI_SIO_TIMER_NO)
                                            // Timer Interrupt Flag
#define REG_MULTI_SIO_TIMER         (REG_BASE + 0x100 + MULTI_SIO_TIMER_NO * 4)
#define REG_MULTI_SIO_TIMER_L        REG_MULTI_SIO_TIMER
#define REG_MULTI_SIO_TIMER_H       (REG_MULTI_SIO_TIMER + 2)
                                            // Timer Register


#define MULTI_SIO_TIMER_COUNT       0xB1FC
                                            // Timer Count


// Multi-play Communication Packet Structure
typedef struct {
    u8  FrameCounter;                       // Frame Counter
    u8  RecvErrorFlags;                     // Receive Error Flag
    u16 CheckSum;                           // Checksum
    u16 Data[MULTI_SIO_BLOCK_SIZE/2];       // Communication Data
    u16 OverRunCatch[2];                    // Overrun Protect Area
} MultiSioPacket;


// Multi-play Communication Work Area Structure
typedef struct {
    u8  Type;                               // Connection (Master/Slave)
    u8  State;                              // Communication Function State
    u8  ConnectedFlags;                     // Connection History Flag
    u8  RecvSuccessFlags;                   // Receive Success Flag

    u8  SyncRecvFlag[4];                    // Receive Confirmation Flag

    u8  StartFlag;                          // Communication Start Flag

    s8  Padding0[2];

    u8  SendFrameCounter;                   // Send Frame Counter
    u8  RecvFrameCounter[4][2];             // Receive Frame Counter

    s32 SendBufCounter;                     // Send Buffer Counter
    s32 RecvBufCounter[4];                  // Receive Buffer Counter

    u16 *NextSendBufp;                      // Send Buffer Pointer
    u16 *CurrentSendBufp;
    u16 *CurrentRecvBufp[4];                // Receive Buffer Pointer
    u16 *LastRecvBufp[4];
    u16 *RecvCheckBufp[4];

    MultiSioPacket  SendBuf[2];             // Send Buffer (Double Buffer)
    MultiSioPacket  RecvBuf[MULTI_SIO_PLAYERS_MAX][3];
                                            // Receive Buffer (Triple Buffer)
} MultiSioArea;


extern u32 RecvFuncBuf[];                   // CPU Internal RAM Execution Buffer
extern u32 IntrFuncBuf[];

extern MultiSioArea     Ms;                 // Multi-play Communication Work Area

/*------------------------------------------------------------------*/
/*                      Multi-play Communication Initialization     */
/*------------------------------------------------------------------*/

extern void MultiSioInit(void);

//* Set serial communication mode to multi-play mode.
//* Initialize register and buffer.

/*------------------------------------------------------------------*/
/*                      Multi-play Communication Main               */
/*------------------------------------------------------------------*/

extern u32  MultiSioMain(void *Sendp, void *Recvp);

//* First determine if master or slave. If master recognized, initialize
//  timer.
//* Call MultiSioSendDataSet() and set send data.
//* Call MultiSioRecvDataCheck() and check if normal receive done, 
//  and copy receive data to Recvp.
//
//* Set so called with as close timing as possible within 1 frame.
//* Safer not to send data that matches flag data (SIO_SYNC_DATA) prior 
//  to connection determination.
//
//* Arguments:
//    void *Sendp  User Send Buffer Pointer
//    void *Recvp  User Receive Buffer Pointer

//* Return Value:

#define MULTI_SIO_RECV_ID_MASK      0x000f  // Receive Success Flag
#define MULTI_SIO_CONNECTED_ID_MASK 0x0f00  // Connection History Flag

#define MULTI_SIO_RECV_ID0          0x0001  // Receive Success Flag Master
#define MULTI_SIO_RECV_ID1          0x0002  //                Slave 1
#define MULTI_SIO_RECV_ID2          0x0004  //                Slave 2
#define MULTI_SIO_RECV_ID3          0x0008  //                Slave 3
#define MULTI_SIO_TYPE              0x0080  // Connection (Master/Slave)
#define MULTI_SIO_PARENT            0x0080  // Master Connection
#define MULTI_SIO_CHILD             0x0000  // Slave Connection
#define MULTI_SIO_CONNECTED_ID0     0x0100  // Connection History Flag Master
#define MULTI_SIO_CONNECTED_ID1     0x0200  //                Slave 1
#define MULTI_SIO_CONNECTED_ID2     0x0400  //                Slave 2
#define MULTI_SIO_CONNECTED_ID3     0x0800  //                Slave 3


// Return Value Structure
typedef struct {
    u32 RecvSuccessFlags:4;                 // Receive Success Flag
    u32 Reserved_0:3;                       // Reserved
    u32 Type:1;                             // Connection (Master/Slave)
    u32 ConnectedFlags:4;                   // Connection History Flag
} MultiSioReturn;



/*------------------------------------------------------------------*/
/*                      Multi-play Communication Interrupt          */
/*------------------------------------------------------------------*/

extern void MultiSioIntr(void);

//* During communication interrupt, store receive data from each unit
//  in each receive buffer and set the send buffer data to the register.
//* If master, reset timer and restart send.
//
//* Program so slave is called with communication interrupt and master
//  is called with timer interrupt.
//* Adjust setting so 1 packet (Except for OverRunCatch[]) can be 
//  transfered with 1 frame.


/*------------------------------------------------------------------*/
/*                      Set Send Data                               */
/*------------------------------------------------------------------*/

extern void MultiSioSendDataSet(void *Sendp);

//* Set the user send buffer data to send buffer.
//
//* Called from MultiSioMain().
//* Not necessary to call directly.
//
//* Arguments:
//    void *Sendp  User Send Buffer Pointer

/*------------------------------------------------------------------*/
/*                      Check Receive Data                          */
/*------------------------------------------------------------------*/

extern u32  MultiSioRecvDataCheck(void *Recvp);

//* Check if receive done normally. If normal, copy the receive data
//  to the user receive buffer.
//
//* Called from MultiSioMain().
//* Do not need to call directly.
//
//Arguments:
//    void *Recvp  User Receive Buffer Pointer

#endif