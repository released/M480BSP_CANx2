/*_____ I N C L U D E S ____________________________________________________*/
#include <stdio.h>
#include <string.h>
#include "NuMicro.h"

#include "project_config.h"


/*_____ D E C L A R A T I O N S ____________________________________________*/

/*_____ D E F I N I T I O N S ______________________________________________*/
volatile uint32_t BitFlag = 0;
volatile uint32_t counter_tick = 0;

STR_CANMSG_T rrMsg = {0};
uint8_t Remote_Frame_Receive_Flag = 0;

/*_____ M A C R O S ________________________________________________________*/

#define CAN_DEVICE_A						(CAN0)	// emulate host
#define CAN_DEVICE_B						(CAN1)

#define CAN_MSG_ID00						(0x301)	// CAN_STD_ID
#define CAN_MSG_ID01						(0xF502)

#define CAN_MSG_ID02						(0x401)	// CAN_STD_ID
#define CAN_MSG_ID03						(0xF504)

#define CAN_MSG_REMOTE_ID00					(0x501)	// CAN_STD_ID
#define CAN_MSG_REMOTE_ID01					(0xF8EE)

#define CAN_MSG_REMOTE_ID02					(0x601)	// CAN_STD_ID
#define CAN_MSG_REMOTE_ID03					(0xFAEE)

/*_____ F U N C T I O N S __________________________________________________*/
extern int32_t CAN_SetRxMsg_Remote(CAN_T *tCAN, uint32_t u32MsgNum, uint32_t u32IDType, uint32_t u32ID);

void tick_counter(void)
{
	counter_tick++;
}

uint32_t get_tick(void)
{
	return (counter_tick);
}

void set_tick(uint32_t t)
{
	counter_tick = t;
}

void compare_buffer(uint8_t *src, uint8_t *des, int nBytes)
{
    uint16_t i = 0;	
	
    for (i = 0; i < nBytes; i++)
    {
        if (src[i] != des[i])
        {
            printf("error idx : %4d : 0x%2X , 0x%2X\r\n", i , src[i],des[i]);
			set_flag(flag_error , ENABLE);
        }
    }

	if (!is_flag_set(flag_error))
	{
    	printf("%s finish \r\n" , __FUNCTION__);	
		set_flag(flag_error , DISABLE);
	}

}

void reset_buffer(void *dest, unsigned int val, unsigned int size)
{
    uint8_t *pu8Dest;
//    unsigned int i;
    
    pu8Dest = (uint8_t *)dest;

	#if 1
	while (size-- > 0)
		*pu8Dest++ = val;
	#else
	memset(pu8Dest, val, size * (sizeof(pu8Dest[0]) ));
	#endif
	
}

void copy_buffer(void *dest, void *src, unsigned int size)
{
    uint8_t *pu8Src, *pu8Dest;
    unsigned int i;
    
    pu8Dest = (uint8_t *)dest;
    pu8Src  = (uint8_t *)src;


	#if 0
	  while (size--)
	    *pu8Dest++ = *pu8Src++;
	#else
    for (i = 0; i < size; i++)
        pu8Dest[i] = pu8Src[i];
	#endif
}

void dump_buffer(uint8_t *pucBuff, int nBytes)
{
    uint16_t i = 0;
    
    printf("dump_buffer : %2d\r\n" , nBytes);    
    for (i = 0 ; i < nBytes ; i++)
    {
        printf("0x%2X," , pucBuff[i]);
        if ((i+1)%8 ==0)
        {
            printf("\r\n");
        }            
    }
    printf("\r\n\r\n");
}

void  dump_buffer_hex(uint8_t *pucBuff, int nBytes)
{
    int     nIdx, i;

    nIdx = 0;
    while (nBytes > 0)
    {
        printf("0x%04X  ", nIdx);
        for (i = 0; i < 16; i++)
            printf("%02X ", pucBuff[nIdx + i]);
        printf("  ");
        for (i = 0; i < 16; i++)
        {
            if ((pucBuff[nIdx + i] >= 0x20) && (pucBuff[nIdx + i] < 127))
                printf("%c", pucBuff[nIdx + i]);
            else
                printf(".");
            nBytes--;
        }
        nIdx += 16;
        printf("\n");
    }
    printf("\n");
}

void Delay(uint32_t delay)
{
	uint32_t i;
	
	for(i=0; i<delay; i++)
	{
		CLK_SysTickDelay(1000);//1ms
	}
}


void CAN_ShowMsg(STR_CANMSG_T* Msg)
{
    uint8_t i;

    UART_WAIT_TX_EMPTY(UART0);	
	//printf("\n\n");
	printf("MsgID = 0x%04X ,", Msg->Id);
	printf("Type = %s(0x%02X) ,",Msg->IdType?"EXT":"STD" ,Msg->IdType);
	printf("Frame = %s(0x%02X) ,", Msg->FrameType?"DATA_FRAME":"REMOTE_FRAME" , Msg->FrameType);
	printf("length = 0x%02X \r\n", Msg->DLC);
	printf("Data is: ");
	for(i=0; i<Msg->DLC; i++)
	{
		printf("0x%02X ,",Msg->Data[i]);
	}
	printf("\r\n");

}


void CAN_MsgInterrupt(CAN_T *tCAN, uint32_t u32IIDR)
{
	uint32_t u32MsgNum = 0;
	
	if(u32IIDR <= 32)
	{
		u32MsgNum = (u32IIDR-1);
		
		CAN_Receive(tCAN, u32MsgNum,&rrMsg);

		if ((rrMsg.FrameType==CAN_DATA_FRAME))
		{
			// printf RX only
			if  (!( (u32MsgNum == 6) || 
				(u32MsgNum == 7) || 
				(u32MsgNum == 22) || 
				(u32MsgNum == 23) || 
				(u32MsgNum == 4) || 
				(u32MsgNum == 5) || 
				(u32MsgNum == 20) || 
				(u32MsgNum == 21) ))
			{
				printf("%s -Num:%2d \r\n", (tCAN == CAN0) ? "HostA" : "DeviceB"  ,u32MsgNum);		
				CAN_ShowMsg(&rrMsg);
			}
		}
		
		if((rrMsg.FrameType==CAN_REMOTE_FRAME) && ((u32MsgNum == 18)) )
		{
			printf("\r\n%s (Remote)-Num:%2d ,ID:0x%04X , %s(0x%02X) , len:0x%02X\r\n", (tCAN == CAN0) ? "HostA" : "DeviceB"  ,u32MsgNum, CAN_MSG_REMOTE_ID02,rrMsg.IdType?"EXT":"STD" ,rrMsg.IdType , rrMsg.DLC );			
		}		
		if((rrMsg.FrameType==CAN_REMOTE_FRAME) && ((u32MsgNum == 19)) )
		{
			printf("\r\n%s (Remote)-Num:%2d ,ID:0x%04X , %s(0x%02X) , len:0x%02X\r\n", (tCAN == CAN0) ? "HostA" : "DeviceB"  ,u32MsgNum, CAN_MSG_REMOTE_ID03,rrMsg.IdType?"EXT":"STD" ,rrMsg.IdType , rrMsg.DLC );			
		}	
		if((rrMsg.FrameType==CAN_REMOTE_FRAME) && ((u32MsgNum == 2) ) )
		{
			printf("\r\n%s (Remote)-Num:%2d ,ID:0x%04X , %s(0x%02X) , len:0x%02X\r\n", (tCAN == CAN0) ? "HostA" : "DeviceB"  ,u32MsgNum, CAN_MSG_REMOTE_ID00,rrMsg.IdType?"EXT":"STD" ,rrMsg.IdType , rrMsg.DLC );			
		}		
		if((rrMsg.FrameType==CAN_REMOTE_FRAME) && ((u32MsgNum == 3) ) )
		{
			printf("\r\n%s (Remote)-Num:%2d ,ID:0x%04X , %s(0x%02X) , len:0x%02X\r\n", (tCAN == CAN0) ? "HostA" : "DeviceB"  ,u32MsgNum, CAN_MSG_REMOTE_ID01,rrMsg.IdType?"EXT":"STD" ,rrMsg.IdType , rrMsg.DLC );
		}

//		printf("\r\n");
	}
}

void CAN_IRQHandler(CAN_T *tCAN)
{
    uint32_t u8IIDRstatus;

    u8IIDRstatus = CAN_GET_INT_PENDING_STATUS(tCAN);

    if(u8IIDRstatus == 0x00008000)        /* Check Status Interrupt Flag (Error status Int and Status change Int) */
    {
        /**************************/
        /* Status Change interrupt*/
        /**************************/
        if(tCAN->STATUS & CAN_STATUS_RXOK_Msk)
        {
            tCAN->STATUS &= ~CAN_STATUS_RXOK_Msk;   /* Clear Rx Ok status*/

            printf("RX OK INT\n") ;
        }

        if(tCAN->STATUS & CAN_STATUS_TXOK_Msk)
        {
            tCAN->STATUS &= ~CAN_STATUS_TXOK_Msk;    /* Clear Tx Ok status*/

            printf("TX OK INT\n") ;
        }

        /**************************/
        /* Error Status interrupt */
        /**************************/
        if(tCAN->STATUS & CAN_STATUS_EWARN_Msk)
        {
            printf("EWARN INT\n") ;
        }

        if(tCAN->STATUS & CAN_STATUS_BOFF_Msk)
        {
            printf("BOFF INT\n") ;

            /* Do Init to release busoff pin */
            tCAN->CON = (CAN_CON_INIT_Msk | CAN_CON_CCE_Msk);
            tCAN->CON &= (~(CAN_CON_INIT_Msk | CAN_CON_CCE_Msk));
            while(tCAN->CON & CAN_CON_INIT_Msk);
        }
    }
    else if (u8IIDRstatus!=0)
    {
//        printf("=> Interrupt Pointer = %d\n",CAN_GET_INT_PENDING_STATUS(tCAN) -1);
//        printf("=> Interrupt Pointer = %d\n",u8IIDRstatus -1);

        CAN_MsgInterrupt(tCAN, u8IIDRstatus);

//        CAN_CLR_INT_PENDING_BIT(tCAN, (CAN_GET_INT_PENDING_STATUS(tCAN) -1));      /* Clear Interrupt Pending */
		CAN_CLR_INT_PENDING_BIT(tCAN, (u8IIDRstatus -1)); 

    }
    else if(tCAN->WU_STATUS == 1)
    {
        printf("Wake up\n");

        tCAN->WU_STATUS = 0;                       /* Write '0' to clear */
    }

}



/**
  * @brief  CAN0_IRQ Handler.
  * @param  None.
  * @return None.
  */
void CAN0_IRQHandler(void)
{
//	printf("%s \r\n",__FUNCTION__);
	CAN_IRQHandler(CAN_DEVICE_A);

}

/**
  * @brief  CAN1_IRQ Handler.
  * @param  None.
  * @return None.
  */
void CAN1_IRQHandler(void)
{
//	printf("%s \r\n",__FUNCTION__);
	CAN_IRQHandler(CAN_DEVICE_B);

}


void DEVICE_B_SetRxMsg(CAN_T *tCAN)
{
    if(CAN_SetRxMsg(tCAN, MSG(16),CAN_STD_ID, CAN_MSG_ID02) == FALSE)
    {
        printf("Set Rx Msg Object failed\n");
        return;
    }

    if(CAN_SetRxMsg(tCAN, MSG(17),CAN_EXT_ID, CAN_MSG_ID03) == FALSE)
    {
        printf("Set Rx Msg Object failed\n");
        return;
    }

    if(CAN_SetRxMsg_Remote(tCAN, MSG(18),CAN_STD_ID, CAN_MSG_REMOTE_ID02) == FALSE)
    {
        printf("Set Rx Msg Object failed\n");
        return;
    }

	if(CAN_SetRxMsg_Remote(tCAN, MSG(19),CAN_EXT_ID, CAN_MSG_REMOTE_ID03) == FALSE)
    {
        printf("Set Rx Msg Object failed\n");
        return;
    }

    
	CAN_EnableInt(tCAN, CAN_CON_IE_Msk);
//    NVIC_SetPriority(CAN1_IRQn, (1<<__NVIC_PRIO_BITS) - 2);
    NVIC_SetPriority(CAN1_IRQn, 0);
    NVIC_EnableIRQ(CAN1_IRQn);

}


void DEVICE_A_SetRxMsg(CAN_T *tCAN)
{
    if(CAN_SetRxMsg(tCAN, MSG(0),CAN_STD_ID, CAN_MSG_ID00) == FALSE)
    {
        printf("Set Rx Msg Object failed\n");
        return;
    }

    if(CAN_SetRxMsg(tCAN, MSG(1),CAN_EXT_ID, CAN_MSG_ID01) == FALSE)
    {
        printf("Set Rx Msg Object failed\n");
        return;
    }

    if(CAN_SetRxMsg_Remote(tCAN, MSG(2),CAN_STD_ID, CAN_MSG_REMOTE_ID00) == FALSE)
    {
        printf("Set Rx Msg Object failed\n");
        return;
    }

	if(CAN_SetRxMsg_Remote(tCAN, MSG(3),CAN_EXT_ID, CAN_MSG_REMOTE_ID01) == FALSE)
    {
        printf("Set Rx Msg Object failed\n");
        return;
    }


	CAN_EnableInt(tCAN, CAN_CON_IE_Msk);
//    NVIC_SetPriority(CAN0_IRQn, (1<<__NVIC_PRIO_BITS) - 2);
    NVIC_SetPriority(CAN0_IRQn, 0);
    NVIC_EnableIRQ(CAN0_IRQn);


}

void DEVICE_B_SetTxMsg_Remote_Frame(CAN_T *tCAN)
{
    STR_CANMSG_T tMsg;

	printf("B_Tx Remote start >>> \r\n");

	tMsg.FrameType= CAN_REMOTE_FRAME;
	tMsg.IdType   = CAN_STD_ID;
	tMsg.Id       = CAN_MSG_REMOTE_ID00;
	tMsg.DLC      = 6;

	if(CAN_Transmit(tCAN, MSG(20),&tMsg) == FALSE)   // Configure Msg RAM and send the Msg in the RAM
	{
		printf("Set Tx Msg Object failed\n");
		return;
	}

	tMsg.FrameType= CAN_REMOTE_FRAME;
	tMsg.IdType   = CAN_EXT_ID;
	tMsg.Id       = CAN_MSG_REMOTE_ID01;
	tMsg.DLC      = 8;

	if(CAN_Transmit(tCAN, MSG(21),&tMsg) == FALSE)   // Configure Msg RAM and send the Msg in the RAM
	{
		printf("Set Tx Msg Object failed\n");
		return;
	}	

    UART_WAIT_TX_EMPTY(UART0);	
	printf("B_Tx Remote finish ### \r\n\r\n");   
	
}

void DEVICE_B_Tx(CAN_T *tCAN)
{
    STR_CANMSG_T tMsg;
    uint32_t i;
	const uint8_t can_data_00[5] = {0x07 , 0xFF , 0x01 , 0x5A , 0xA5};
	const uint8_t can_data_01[5] = {0x07 , 0xFF , 0x02 , 0x5A , 0xA5};

	printf("B_Tx  start >>> \r\n");
	
    /* Send a 11-bits message */
    tMsg.FrameType= CAN_DATA_FRAME;
    tMsg.IdType   = CAN_STD_ID;
    tMsg.Id       = CAN_MSG_ID00;
    tMsg.DLC      = strlen((char*) can_data_00);	//SIZEOF(can_data_00);	//2;
	for ( i = 0 ; i < 5 ; i++)
	{
		tMsg.Data[i]  = can_data_00[i];
	}
    if(CAN_Transmit(tCAN, MSG(22),&tMsg) == FALSE) 
    {
        printf("Set Tx Msg Object failed\n");
        return;
    }

    /* Send a 29-bits message */
    tMsg.FrameType= CAN_DATA_FRAME;
    tMsg.IdType   = CAN_EXT_ID;
    tMsg.Id       = CAN_MSG_ID01;
    tMsg.DLC      = strlen((char*) can_data_01);	//SIZEOF(can_data_01);	//3;
	for ( i = 0 ; i < 5 ; i++)
	{
		tMsg.Data[i]  = can_data_01[i];
	}

    if(CAN_Transmit(tCAN, MSG(23),&tMsg) == FALSE)
    {
        printf("Set Tx Msg Object failed\n");
        return;
    }
    
    UART_WAIT_TX_EMPTY(UART0);		
	printf("B_Tx finish ### \r\n\r\n");   

}

void DEVICE_A_SetTxMsg_Remote_Frame(CAN_T *tCAN)
{
    STR_CANMSG_T tMsg;
    
	printf("A_Tx  Remote start >>> \r\n");

	tMsg.FrameType= CAN_REMOTE_FRAME;
	tMsg.IdType   = CAN_STD_ID;
	tMsg.Id       = CAN_MSG_REMOTE_ID02;
	tMsg.DLC      = 2;

	if(CAN_Transmit(tCAN, MSG(4),&tMsg) == FALSE)   // Configure Msg RAM and send the Msg in the RAM
	{
		printf("Set Tx Msg Object failed\n");
		return;
	}

	tMsg.FrameType= CAN_REMOTE_FRAME;
	tMsg.IdType   = CAN_EXT_ID;
	tMsg.Id       = CAN_MSG_REMOTE_ID03;
	tMsg.DLC      = 4;

	if(CAN_Transmit(tCAN, MSG(5),&tMsg) == FALSE)   // Configure Msg RAM and send the Msg in the RAM
	{
		printf("Set Tx Msg Object failed\n");
		return;
	}	
	
    UART_WAIT_TX_EMPTY(UART0);	
	printf("A_Tx  Remote finish ### \r\n\r\n");   
	
}


void DEVICE_A_Tx(CAN_T *tCAN)
{
    STR_CANMSG_T tMsg;
    
	printf("A_Tx  start >>> \r\n");
	
	/* Send a 11-bits message */
    tMsg.FrameType= CAN_DATA_FRAME;
    tMsg.IdType   = CAN_STD_ID;
    tMsg.Id       = CAN_MSG_ID02;
    tMsg.DLC      = 2;
    tMsg.Data[0]  = 0x01;
    tMsg.Data[1]  = 0x04;

    if(CAN_Transmit(tCAN, MSG(6),&tMsg) == FALSE)   // Configure Msg RAM and send the Msg in the RAM
    {
        printf("Set Tx Msg Object failed\n");
        return;
    }

    tMsg.FrameType= CAN_DATA_FRAME;
    tMsg.IdType   = CAN_EXT_ID;
    tMsg.Id       = CAN_MSG_ID03;
    tMsg.DLC      = 2;
    tMsg.Data[0]  = 0x04;
    tMsg.Data[1]  = 0xF5;

    if(CAN_Transmit(tCAN, MSG(7),&tMsg) == FALSE)   // Configure Msg RAM and send the Msg in the RAM
    {
        printf("Set Tx Msg Object failed\n");
        return;
    }
    UART_WAIT_TX_EMPTY(UART0);	
	printf("A_Tx finish ### \r\n\r\n");   
	
}


void CAN_Setup(void)
{
    DEVICE_A_SetRxMsg(CAN_DEVICE_A);
    DEVICE_B_SetRxMsg(CAN_DEVICE_B);

    DEVICE_A_Tx(CAN_DEVICE_A);
    DEVICE_B_Tx(CAN_DEVICE_B);
    
	DEVICE_A_SetTxMsg_Remote_Frame(CAN_DEVICE_A);    
	DEVICE_B_SetTxMsg_Remote_Frame(CAN_DEVICE_B);

}

void CAN_Init(void)
{
    CAN_Open(CAN0,  500000, CAN_NORMAL_MODE);
    CAN_Open(CAN1,  500000, CAN_NORMAL_MODE);

}

void Test_flow(void)
{
	
	if(Remote_Frame_Receive_Flag == 1)
	{
		Remote_Frame_Receive_Flag = 0;
	

	}
}


void TMR1_IRQHandler(void)
{
//	static uint32_t LOG = 0;

	
    if(TIMER_GetIntFlag(TIMER1) == 1)
    {
        TIMER_ClearIntFlag(TIMER1);
		tick_counter();

		if ((get_tick() % 1000) == 0)
		{
//        	printf("%s : %4d\r\n",__FUNCTION__,LOG++);
			PH0 ^= 1;
		}

		if ((get_tick() % 50) == 0)
		{

		}	
    }
}


void TIMER1_Init(void)
{
    TIMER_Open(TIMER1, TIMER_PERIODIC_MODE, 1000);
    TIMER_EnableInt(TIMER1);
    NVIC_EnableIRQ(TMR1_IRQn);	
    TIMER_Start(TIMER1);
}

void UARTx_Process(void)
{
	uint8_t res = 0;
	res = UART_READ(UART0);

	if (res == 'x' || res == 'X')
	{
		NVIC_SystemReset();
	}

	if (res > 0x7F)
	{
		printf("invalid command\r\n");
	}
	else
	{
		switch(res)
		{
			case '1':
				DEVICE_B_SetTxMsg_Remote_Frame(CAN_DEVICE_B);
				break;
			case '2':
				DEVICE_B_Tx(CAN_DEVICE_B);
				break;

			case 'X':
			case 'x':
			case 'Z':
			case 'z':
				NVIC_SystemReset();		
				break;
		}
	}
}


void UART0_IRQHandler(void)
{
    if(UART_GET_INT_FLAG(UART0, UART_INTSTS_RDAINT_Msk | UART_INTSTS_RXTOINT_Msk))     /* UART receive data available flag */
    {
        while(UART_GET_RX_EMPTY(UART0) == 0)
        {
			UARTx_Process();
        }
    }

    if(UART0->FIFOSTS & (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk | UART_FIFOSTS_PEF_Msk | UART_FIFOSTS_RXOVIF_Msk))
    {
        UART_ClearIntFlag(UART0, (UART_INTSTS_RLSINT_Msk| UART_INTSTS_BUFERRINT_Msk));
    }	
}

void UART0_Init(void)
{
    SYS_ResetModule(UART0_RST);

    /* Configure UART0 and set UART0 baud rate */
    UART_Open(UART0, 115200);

	/* Set UART receive time-out */
	UART_SetTimeoutCnt(UART0, 20);

	UART0->FIFO &= ~UART_FIFO_RFITL_4BYTES;
	UART0->FIFO |= UART_FIFO_RFITL_8BYTES;

	/* Enable UART Interrupt - */
	UART_ENABLE_INT(UART0, UART_INTEN_RDAIEN_Msk | UART_INTEN_TOCNTEN_Msk | UART_INTEN_RXTOIEN_Msk);
	
	NVIC_EnableIRQ(UART0_IRQn);

	#if (_debug_log_UART_ == 1)	//debug
	printf("\r\nCLK_GetCPUFreq : %8d\r\n",CLK_GetCPUFreq());
	printf("CLK_GetHXTFreq : %8d\r\n",CLK_GetHXTFreq());
	printf("CLK_GetLXTFreq : %8d\r\n",CLK_GetLXTFreq());	
	printf("CLK_GetPCLK0Freq : %8d\r\n",CLK_GetPCLK0Freq());
	printf("CLK_GetPCLK1Freq : %8d\r\n",CLK_GetPCLK1Freq());	

//    printf("Product ID 0x%8X\n", SYS->PDID);
	
	#endif
}

void Custom_Init(void)
{
	SYS->GPH_MFPL = (SYS->GPH_MFPL & ~(SYS_GPH_MFPL_PH0MFP_Msk)) | (SYS_GPH_MFPL_PH0MFP_GPIO);
	SYS->GPH_MFPL = (SYS->GPH_MFPL & ~(SYS_GPH_MFPL_PH1MFP_Msk)) | (SYS_GPH_MFPL_PH1MFP_GPIO);
	SYS->GPH_MFPL = (SYS->GPH_MFPL & ~(SYS_GPH_MFPL_PH2MFP_Msk)) | (SYS_GPH_MFPL_PH2MFP_GPIO);

	//EVM LED
	GPIO_SetMode(PH,BIT0,GPIO_MODE_OUTPUT);
	GPIO_SetMode(PH,BIT1,GPIO_MODE_OUTPUT);
	GPIO_SetMode(PH,BIT2,GPIO_MODE_OUTPUT);
	
}

void SYS_Init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Set XT1_OUT(PF.2) and XT1_IN(PF.3) to input mode */
    PF->MODE &= ~(GPIO_MODE_MODE2_Msk | GPIO_MODE_MODE3_Msk);

    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);
    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);

//    CLK_EnableXtalRC(CLK_PWRCTL_LIRCEN_Msk);
//    CLK_WaitClockReady(CLK_STATUS_LIRCSTB_Msk);

//    CLK_EnableXtalRC(CLK_PWRCTL_LXTEN_Msk);
//    CLK_WaitClockReady(CLK_STATUS_LXTSTB_Msk);

    /* Set core clock as PLL_CLOCK from PLL */
    CLK_SetCoreClock(FREQ_192MHZ);
    /* Set PCLK0/PCLK1 to HCLK/2 */
    CLK->PCLKDIV = (CLK_PCLKDIV_APB0DIV_DIV2 | CLK_PCLKDIV_APB1DIV_DIV2);

    /* Enable UART clock */
    CLK_EnableModuleClock(UART0_MODULE);
    /* Select UART clock source from HXT */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HIRC, CLK_CLKDIV0_UART0(1));

    /* Set GPB multi-function pins for UART0 RXD and TXD */
    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB12MFP_Msk | SYS_GPB_MFPH_PB13MFP_Msk);
    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB12MFP_UART0_RXD | SYS_GPB_MFPH_PB13MFP_UART0_TXD);

    CLK_EnableModuleClock(TMR1_MODULE);
    CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_HIRC, 0);

    CLK_EnableModuleClock(CAN0_MODULE);
    CLK_EnableModuleClock(CAN1_MODULE);

    /* Set PA multi-function pins for CAN0 RXD(PA.4) and TXD(PA.5) */
    SYS->GPA_MFPL = (SYS->GPA_MFPL & ~(SYS_GPA_MFPL_PA4MFP_Msk | SYS_GPA_MFPL_PA5MFP_Msk)) |
                    (SYS_GPA_MFPL_PA4MFP_CAN0_RXD | SYS_GPA_MFPL_PA5MFP_CAN0_TXD);

    /* Set PE multi-function pins for CAN1 TXD(PE.7) and RXD(PE.6) */
    SYS->GPE_MFPL = (SYS->GPE_MFPL & ~(SYS_GPE_MFPL_PE6MFP_Msk | SYS_GPE_MFPL_PE7MFP_Msk)) |
                    (SYS_GPE_MFPL_PE6MFP_CAN1_RXD | SYS_GPE_MFPL_PE7MFP_CAN1_TXD);


	
    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate SystemCoreClock. */
    SystemCoreClockUpdate();

    /* Lock protected registers */
    SYS_LockReg();
}

/*
 * This is a template project for M480 series MCU. Users could based on this project to create their
 * own application without worry about the IAR/Keil project settings.
 *
 * This template application uses external crystal as HCLK source and configures UART0 to print out
 * "Hello World", users may need to do extra system configuration based on their system design.
 */

int main()
{
    SYS_Init();

	UART0_Init();
	Custom_Init();	
	TIMER1_Init();

	CAN_Init();
	CAN_Setup();

    /* Got no where to go, just loop forever */
    while(1)
    {
		Test_flow();

    }
}

/*** (C) COPYRIGHT 2016 Nuvoton Technology Corp. ***/
