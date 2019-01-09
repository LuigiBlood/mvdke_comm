/*
	Mario VS. Donkey Kong e-Reader Link Commands
*/

#define MVDKE_COMM_MASTER_HANDSHAKE	0x01	//Handshake Request (Master)
#define MVDKE_COMM_MASTER_RDY		0x02	//Ready to receive data
#define MVDKE_COMM_MASTER_REQ_PART	0x03	//0xPP03 (PP = Part Number)

#define MVDKE_COMM_SLAVE_HANDSHAKE	0x05	//Handshake Response (Slave)
#define MVDKE_COMM_SLAVE_DATA_RDY	0x06	//Ready to send data
#define MVDKE_COMM_SLAVE_SEND_PART	0x07	//0xPP07 (PP = Part Number)
#define MVDKE_COMM_SLAVE_SEND_LAST_PART	0x08	//0xPP08 (PP = Part Number)
