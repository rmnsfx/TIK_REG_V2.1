/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */     
#include "fatfs.h"

/* FreeRTOS+UDP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_TCP_server.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
	extern char SDPath[4];	
	extern FATFS SDFatFS;
  FIL testFile;
  uint8_t testBuffer[16] = "SD write success";
  UINT testBytes;
  volatile FRESULT res;
/* USER CODE END Variables */
osThreadId defaultTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */


/*
 * Uses FreeRTOS+TCP to listen for incoming echo connections, creating a task
 * to handle each connection.
 */
static void prvConnectionListeningTask( void *pvParameters );

/*
 * Created by the connection listening task to handle a single connection.
 */
static void prvServerConnectionInstance( void *pvParameters );

#define configECHO_SERVER_RX_WINDOW_SIZE	2
#define configECHO_SERVER_TX_WINDOW_SIZE	2
#define tcpechoPORT_NUMBER		80
#define tcpechoSHUTDOWN_DELAY	( pdMS_TO_TICKS( 5000 ) )


/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];
  
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}                   
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
       
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, prvConnectionListeningTask, osPriorityNormal, 0, 256);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used 
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN StartDefaultTask */
	
//	res = f_mount(&SDFatFS, SDPath, 1);
//	if( res == FR_OK)
//  {
//    uint8_t path[13] = "3file.txt";
//    path[12] = '\0';
// 
//    res = f_open(&testFile, (char*)path, FA_WRITE | FA_CREATE_ALWAYS);
// 
//    res = f_write(&testFile, testBuffer, 16, &testBytes);
// 
//    res = f_close(&testFile);
//  }
#define configHTTP_ROOT "/websrc"
const TickType_t xInitialBlockTime = pdMS_TO_TICKS( 5000UL );	
TCPServer_t *pxTCPServer = NULL;	
static const struct xSERVER_CONFIG xServerConfiguration[] =
{
		/* Server type,		port number,	backlog, 	root dir. */
		{ eSERVER_HTTP, 	80, 			12, 		configHTTP_ROOT },
		/* Server type,		port number,	backlog, 	root dir. */
		{ eSERVER_FTP,  	21, 			12, 		"" }
};
	
pxTCPServer = FreeRTOS_CreateTCPServer( xServerConfiguration, sizeof( xServerConfiguration ) / sizeof( xServerConfiguration[ 0 ] ) );

FreeRTOS_TCPServerWork( pxTCPServer, xInitialBlockTime );
	
  /* Infinite loop */
  for(;;)
  {
		
		HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_13);
    osDelay(300);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* Stores the stack size passed into vStartSimpleTCPServerTasks() so it can be
reused when the server listening task creates tasks to handle connections. */
static uint16_t usUsedStackSize = 0;


static void prvConnectionListeningTask( void *pvParameters )
{
struct freertos_sockaddr xClient, xBindAddress;
Socket_t xListeningSocket, xConnectedSocket;
socklen_t xSize = sizeof( xClient );
static const TickType_t xReceiveTimeOut = portMAX_DELAY;
const BaseType_t xBacklog = 1;
volatile char *pucRxBuffer;
BaseType_t xReturned;
TaskHandle_t xHandle = NULL;

#if( ipconfigUSE_TCP_WIN == 1 )
	WinProperties_t xWinProps;

	/* Fill in the buffer and window sizes that will be used by the socket. */
//	xWinProps.lTxBufSize = ipconfigTCP_TX_BUFFER_LENGTH;
//	xWinProps.lTxWinSize = configECHO_SERVER_TX_WINDOW_SIZE;
//	xWinProps.lRxBufSize = ipconfigTCP_RX_BUFFER_LENGTH;
//	xWinProps.lRxWinSize = configECHO_SERVER_RX_WINDOW_SIZE;
	
	xWinProps.lTxBufSize = 24 * ipconfigTCP_MSS;
	xWinProps.lTxWinSize = 8;
	xWinProps.lRxBufSize = 24 * ipconfigTCP_MSS;
	xWinProps.lRxWinSize = 8;
	
#endif /* ipconfigUSE_TCP_WIN */

	/* Just to prevent compiler warnings. */
	( void ) pvParameters;

	/* Attempt to open the socket. */
	xListeningSocket = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP );
	configASSERT( xListeningSocket != FREERTOS_INVALID_SOCKET );

	/* Set a time out so accept() will just wait for a connection. */
	FreeRTOS_setsockopt( xListeningSocket, 0, FREERTOS_SO_RCVTIMEO, &xReceiveTimeOut, sizeof( xReceiveTimeOut ) );

	/* Set the window and buffer sizes. */
	#if( ipconfigUSE_TCP_WIN == 1 )
	{
		FreeRTOS_setsockopt( xListeningSocket, 0, FREERTOS_SO_WIN_PROPERTIES, ( void * ) &xWinProps, sizeof( xWinProps ) );
	}
	#endif /* ipconfigUSE_TCP_WIN */

	/* Bind the socket to the port that the client task will send to, then
	listen for incoming connections. */
	xBindAddress.sin_port = tcpechoPORT_NUMBER;
	xBindAddress.sin_port = FreeRTOS_htons( xBindAddress.sin_port );
	FreeRTOS_bind( xListeningSocket, &xBindAddress, sizeof( xBindAddress ) );
	FreeRTOS_listen( xListeningSocket, xBacklog );

	for( ;; )
	{
		/* Wait for a client to connect. */
		xConnectedSocket = FreeRTOS_accept( xListeningSocket, &xClient, &xSize );
		configASSERT( xConnectedSocket != FREERTOS_INVALID_SOCKET );
			
		prvServerConnectionInstance(( void * ) xConnectedSocket);
		
//		/* Spawn a task to handle the connection. */
//		xReturned = xTaskCreate( prvServerConnectionInstance, "EchoServer", configMINIMAL_STACK_SIZE, ( void * ) xConnectedSocket, tskIDLE_PRIORITY + 1, &xHandle );
//		
//		if( xReturned != pdPASS )
//    {
//			for(;;)
//			{
//					HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_13);
//					osDelay(200);
//			}
//    }
		
		
	}
}




static void prvServerConnectionInstance( void *pvParameters )
{
int32_t lBytes, lSent, lTotalSent;
Socket_t xConnectedSocket;
static const TickType_t xReceiveTimeOut = pdMS_TO_TICKS( 50000 );
static const TickType_t xSendTimeOut = pdMS_TO_TICKS( 50000 );
TickType_t xTimeOnShutdown;
uint8_t *pucRxBuffer;

	xConnectedSocket = ( Socket_t ) pvParameters;

	/* Attempt to create the buffer used to receive the string to be echoed
	back.  This could be avoided using a zero copy interface that just returned
	the same buffer. */
	pucRxBuffer = ( uint8_t * ) pvPortMalloc( ipconfigTCP_MSS );

	if( pucRxBuffer != NULL )
	{
		FreeRTOS_setsockopt( xConnectedSocket, 0, FREERTOS_SO_RCVTIMEO, &xReceiveTimeOut, sizeof( xReceiveTimeOut ) );
		FreeRTOS_setsockopt( xConnectedSocket, 0, FREERTOS_SO_SNDTIMEO, &xSendTimeOut, sizeof( xReceiveTimeOut ) );

		for( ;; )
		{
			/* Zero out the receive array so there is NULL at the end of the string
			when it is printed out. */
			memset( pucRxBuffer, 0x00, ipconfigTCP_MSS );

			/* Receive data on the socket. */
			lBytes = FreeRTOS_recv( xConnectedSocket, pucRxBuffer, ipconfigTCP_MSS, 0 );
						
			/* If data was received, echo it back. */
			if( lBytes >= 0 )
			{
				lSent = 0;
				lTotalSent = 0;

				/* Call send() until all the data has been sent. */
				while( ( lSent >= 0 ) && ( lTotalSent < lBytes ) )
				{
					lSent = FreeRTOS_send( xConnectedSocket, pucRxBuffer, lBytes - lTotalSent, 0 );
					lTotalSent += lSent;
				}

				if( lSent < 0 )
				{
					/* Socket closed? */
					break;
				}
			}
			else
			{
				/* Socket closed? */
				break;
			}
		}
	}

	/* Initiate a shutdown in case it has not already been initiated. */
	FreeRTOS_shutdown( xConnectedSocket, FREERTOS_SHUT_RDWR );

	/* Wait for the shutdown to take effect, indicated by FreeRTOS_recv()
	returning an error. */
	xTimeOnShutdown = xTaskGetTickCount();
	do
	{
		if( FreeRTOS_recv( xConnectedSocket, pucRxBuffer, ipconfigTCP_MSS, 0 ) < 0 )
		{
			break;
		}
	} while( ( xTaskGetTickCount() - xTimeOnShutdown ) < tcpechoSHUTDOWN_DELAY );

	/* Finished with the socket, buffer, the task. */
	vPortFree( pucRxBuffer );
	FreeRTOS_closesocket( xConnectedSocket );

	//vTaskDelete( NULL );
}


/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
