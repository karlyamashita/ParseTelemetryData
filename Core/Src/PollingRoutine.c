/*
 * PollingRoutine.c
 *
 *  Created on: Oct 24, 2023
 *      Author: karl.yamashita
 *
 *
 *      Template for projects.
 *
 */


#include "main.h"

extern UART_HandleTypeDef hlpuart1;

#define QUEUE_SIZE 2
#define DATA_SIZE 10

#define RING_BUFF_OVERFLOW_SIZE 100

typedef struct {
	uint32_t index_IN; // pointer to where the data will be save to
	uint32_t index_OUT; // pointer to next available data in buffer if cnt_handle is not zero
	uint32_t cnt_Handle; // if not zero then message available
	uint32_t cnt_OverFlow; // has overflow if not zero
	uint32_t queueSize;
}RING_BUFF_STRUCT;

typedef struct
{
	struct
	{
		uint8_t data[DATA_SIZE];
	}Queue[QUEUE_SIZE];
	RING_BUFF_STRUCT ptr;
}UART_data_t;

UART_data_t uart_data =
{
	.ptr.queueSize = QUEUE_SIZE
};


HAL_StatusTypeDef hal_status;

// prototypes
void PollingInit(void);
void PollingRoutine(void);
void Parse(void);
void UART_EnInt(void);
void UART_CheckStatus(void);
void RingBuff_Ptr_Input(RING_BUFF_STRUCT *ptr);
void RingBuff_Ptr_Output(RING_BUFF_STRUCT *ptr);
bool ValidateChkSum(uint8_t *data, uint8_t len);

// called before main while loop
void PollingInit(void)
{
	UART_EnInt();
}

// called from main while loop
void PollingRoutine(void)
{
	UART_CheckStatus();

	Parse();
}

void Parse(void)
{
	if(uart_data.ptr.cnt_Handle)
	{
		if(ValidateChkSum(uart_data.Queue[uart_data.ptr.index_OUT].data, DATA_SIZE))
		{
			if(HAL_UART_Transmit_IT(&hlpuart1, uart_data.Queue[uart_data.ptr.index_OUT].data, DATA_SIZE) == HAL_OK)
			{
				// increment pointer only if successful transmit of pending data buffer
				RingBuff_Ptr_Output(&uart_data.ptr);
			}
		}
	}
}


// enable UART rx interrupt
void UART_EnInt(void)
{
	hal_status = HAL_UARTEx_ReceiveToIdle_IT(&hlpuart1, uart_data.Queue[uart_data.ptr.index_IN].data, DATA_SIZE);
}

// check HAL status
void UART_CheckStatus(void)
{
	if(hal_status != HAL_OK)
	{
		UART_EnInt(); // try to enable again
	}
}

// callback
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if(huart == &hlpuart1)
	{
		if(Size == DATA_SIZE)
		{
			RingBuff_Ptr_Input(&uart_data.ptr);
		}
		UART_EnInt();
	}
}


void RingBuff_Ptr_Input(RING_BUFF_STRUCT *ptr)
{
	ptr->index_IN++;
	if (ptr->index_IN >= ptr->queueSize)
		ptr->index_IN = 0;

	ptr->cnt_Handle++;
	if (ptr->index_IN == ptr->index_OUT) {
		ptr->cnt_OverFlow++;
		if (ptr->cnt_OverFlow > RING_BUFF_OVERFLOW_SIZE)
			ptr->cnt_OverFlow = 0;
		if (ptr->index_IN == 0) {
			ptr->index_OUT = ptr->queueSize - 1;
		} else {
			ptr->index_OUT = ptr->index_IN - 1;
		}
		ptr->cnt_Handle = 1;
	}
}

void RingBuff_Ptr_Output(RING_BUFF_STRUCT *ptr)
{
	if (ptr->cnt_Handle) {
		ptr->index_OUT++;
		if (ptr->index_OUT >= ptr->queueSize)
			ptr->index_OUT = 0;
		ptr->cnt_Handle--;
	}
}

// MOD256
bool ValidateChkSum(uint8_t *data, uint8_t len)
{
    uint32_t chksum = 0;
    uint32_t i = 0;
    for(i = 0; i < len - 1; i++){
        chksum += *data++;
    }
    if((chksum % 256) != *data){
        return false;
    }
    return true;
}


