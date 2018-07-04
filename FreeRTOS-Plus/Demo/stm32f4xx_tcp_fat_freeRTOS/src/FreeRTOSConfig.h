/*
    FreeRTOS V9.0.0 - Copyright (C) 2016 Real Time Engineers Ltd.

    这个文件唯一的包含在FreeRTOS.h中
*/

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *
 * See http://www.freertos.org/a00110.html.
 *----------------------------------------------------------*/


#include <stdint.h>
extern uint32_t SystemCoreClock;

#define configCPU_CLOCK_HZ						( SystemCoreClock )
#define configUSE_PORT_OPTIMISED_TASK_SELECTION	1
#define configUSE_TICKLESS_IDLE					0
#define configTICK_RATE_HZ						( 1000UL )
#define configUSE_PREEMPTION					1  //是否使用抢占式内核
#define configUSE_IDLE_HOOK						1
#define configUSE_TICK_HOOK						1
#define configMAX_PRIORITIES					( 7 )
#define configMINIMAL_STACK_SIZE				( ( uint16_t ) 128 )
#define configTOTAL_HEAP_SIZE					( ( size_t ) 15360 ) /* Not used as heap_5.c is being built, see http://www.freertos.org/a00111.html. */
#define configMAX_TASK_NAME_LEN					( 16 )
#define configUSE_TRACE_FACILITY				1
#define configUSE_16_BIT_TICKS					0
#define configIDLE_SHOULD_YIELD					1
#define configUSE_MUTEXES						1
#define configQUEUE_REGISTRY_SIZE				8
#define configCHECK_FOR_STACK_OVERFLOW			2
#define configUSE_RECURSIVE_MUTEXES				1
#define configUSE_MALLOC_FAILED_HOOK			1
#define configUSE_COUNTING_SEMAPHORES			1
#define configUSE_TASK_NOTIFICATIONS			1
#define configUSE_TICKLESS_IDLE					0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS	3 /* FreeRTOS+FAT requires 3 pointers if a CWD is supported. */
#define configUSE_STATS_FORMATTING_FUNCTIONS	1     //格式化任务状态输出

//使用易失性列表
#define configLIST_VOLATILE                     volatile

/* Co-routine definitions. */
#define configUSE_CO_ROUTINES					0
#define configMAX_CO_ROUTINE_PRIORITIES			( 2 )

/* Software timer definitions. */
#define configUSE_TIMERS						0   //裁剪软件定时器模块
#define configTIMER_TASK_PRIORITY				( configMAX_PRIORITIES - 1 )  //软件定时器的优先级
#define configTIMER_QUEUE_LENGTH				5
#define configTIMER_TASK_STACK_DEPTH			( configMINIMAL_STACK_SIZE * 2 )  //设置任务栈的深度

/* Terminate the task stack from with NULL, rather than the function that
asserts on an invalid task exit.  This is done to stop the GDB debugger getting
confused when trying to unwind a stack frame.  */
#define configTASK_RETURN_ADDRESS				NULL

/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */
#define INCLUDE_vTaskPrioritySet				1         //设置任务优先级
#define INCLUDE_uxTaskPriorityGet				1
#define INCLUDE_vTaskDelete						1
#define INCLUDE_vTaskCleanUpResources			0
#define INCLUDE_vTaskSuspend					1
#define INCLUDE_vTaskDelayUntil					0
#define INCLUDE_vTaskDelay						1
#define INCLUDE_xTimerPendFunctionCall			0
#define INCLUDE_eTaskGetState					1
#define INCLUDE_pcTaskGetTaskName				1
#define INCLUDE_xTaskGetSchedulerState			1

/* FreeRTOS+CLI definitions. */
/* Dimensions a buffer into which command outputs can be written. The buffer
 * can be declared in the CLI code itself, to allow multiple command consoles to
 * share the same buffer. For example, an application may allow access to the
 * command interpreter by UART and by Ethernet. Sharing a buffer is done purely
 * to save RAM. Note, however, that the command console itself is not re-entrant,
 * so only one command interpreter interface can be used at any one time. For
 * that reason, no attempt at providing mutual exclusion to the buffer is
 * attempted.
 */
#define configCOMMAND_INT_MAX_OUTPUT_SIZE		400

/* Run time stats parameters.  http://www.freertos.org/rtos-run-time-stats.html */
#define configGENERATE_RUN_TIME_STATS 			1
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()
extern uint32_t xGetRunTimeCounterValue(void);
#define portGET_RUN_TIME_COUNTER_VALUE()		xGetRunTimeCounterValue()

/* Cortex-M specific definitions.  http://www.freertos.org/RTOS-Cortex-M3-M4.html */
#ifdef __NVIC_PRIO_BITS
	/* __BVIC_PRIO_BITS will be specified when CMSIS is being used. */
 	 #define configPRIO_BITS		__NVIC_PRIO_BITS
#else
	#define configPRIO_BITS		4	/* 15 priority levels */
#endif

/* The lowest interrupt priority that can be used in a call to a "set priority"
function. */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY		15

/* The highest interrupt priority that can be used by any interrupt service
routine that makes calls to interrupt safe FreeRTOS API functions.  DO NOT CALL
INTERRUPT SAFE FREERTOS API FUNCTIONS FROM ANY INTERRUPT THAT HAS A HIGHER
PRIORITY THAN THIS! (higher priorities are lower numeric values. */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5

/* Interrupt priorities used by the kernel port layer itself.  These are generic
to all Cortex-M ports, and do not rely on any particular library functions. */
#define configKERNEL_INTERRUPT_PRIORITY 		( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
/* !!!! configMAX_SYSCALL_INTERRUPT_PRIORITY must not be set to zero !!!!
See http://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html. */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

/* Normal assert() semantics without relying on the provision of an assert.h
header file. */
void vAssertCalled( const char *pcFile, uint32_t ulLine );
#define configASSERT( x ) if( ( x ) == 0 ) vAssertCalled( __FILE__, __LINE__ );

/* Definitions that map the FreeRTOS port interrupt handlers to their CMSIS
standard names. */
#define vPortSVCHandler		SVC_Handler
#define xPortPendSVHandler	PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

/* Defines the port and pin used to detect if an SD card is present.  The
STM3240G-EVAL board uses pin H13 for card detection. */
#define configSD_DETECT_PIN			GPIO_PIN_13
#define configSD_DETECT_GPIO_PORT	GPIOH

/* Default MAC address configuration. */
#define configMAC_ADDR0		0x00
#define configMAC_ADDR1		0x11
#define configMAC_ADDR2		0x22
#define configMAC_ADDR3		0x33
#define configMAC_ADDR4		0x44
#define configMAC_ADDR5		0x46

/* Default IP address configuration.  Used in case ipconfigUSE_DHCP is set to 0,
or ipconfigUSE_DHCP is set to 1 but a DHCP server cannot be contacted. */
#define configIP_ADDR0		192
#define configIP_ADDR1		168
#define configIP_ADDR2		1
#define configIP_ADDR3		200

/* Default gateway IP address configuration.  Used in case ipconfigUSE_DHCP is
set to 0, or ipconfigUSE_DHCP is set to 1 but a DHCP server cannot be contacted. */
#define configGATEWAY_ADDR0	192
#define configGATEWAY_ADDR1	168
#define configGATEWAY_ADDR2	1
#define configGATEWAY_ADDR3	1

/* Default DNS server configuration.  OpenDNS addresses are 208.67.222.222 and
208.67.220.220.  Used in ipconfigUSE_DHCP is set to 0, or ipconfigUSE_DHCP is set
to 1 but a DHCP server cannot be contacted.*/
#define configDNS_SERVER_ADDR0 	208
#define configDNS_SERVER_ADDR1 	67
#define configDNS_SERVER_ADDR2 	222
#define configDNS_SERVER_ADDR3 	222

/* Default netmask configuration.  Used in case ipconfigUSE_DHCP is set to 0,
or ipconfigUSE_DHCP is set to 1 but a DHCP server cannot be contacted. */
#define configNET_MASK0		255
#define configNET_MASK1		255
#define configNET_MASK2		255
#define configNET_MASK3		0

/* The address of an echo server that will be used by the two demo echo client
tasks.
http://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_TCP/TCP_Echo_Clients.html
http://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_TCP/UDP_Echo_Clients.html */
#define configECHO_SERVER_ADDR0	192
#define configECHO_SERVER_ADDR1 168
#define configECHO_SERVER_ADDR2 0
#define configECHO_SERVER_ADDR3 100

/* The address used as the root by the HTTP server. */
#define configHTTP_ROOT "/websrc"

/* If configINCLUDE_DEMO_DEBUG_STATS is set to one, then a few basic IP trace
macros are defined to gather some UDP stack statistics that can then be viewed
through the CLI interface. */
#define configINCLUDE_DEMO_DEBUG_STATS 		1

/* Set configINCLUDE_QUERY_HEAP_COMMAND to 1 to include the query-heap command
in the CLI. */
#define configINCLUDE_QUERY_HEAP_COMMAND 	1

/****** UDP logging settings. *************************************************
See http://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_TCP/UDP_Logging.html */

/* If set to 1 then each message sent via the UDP logging facility will end
with \r\n.  If set to 0 then each message sent via the UDP logging facility will
end with \n. */
#define configUDP_LOGGING_NEEDS_CR_LF  ( 1 )

/* Sets the maximum length for a string sent via the UDP logging facility. */
#define configUDP_LOGGING_STRING_LENGTH	( 200 )

/* The UDP logging facility buffers messages until the UDP logging task is able
to transmit them.  configUDP_LOGGING_MAX_MESSAGES_IN_BUFFER sets the maximum
number of messages that can be buffered at any one time. */
#define	configUDP_LOGGING_MAX_MESSAGES_IN_BUFFER	( 20 )

/* The UDP logging facility creates a task to send buffered messages to the UDP
port.  configUDP_LOGGING_TASK_STACK_SIZE sets the task's stack size. */
#define	configUDP_LOGGING_TASK_STACK_SIZE  	( 512 )

/* The UDP logging facility creates a task to send buffered messages to the UDP
port.  configUDP_LOGGING_TASK_PRIORITY sets the task's priority.  It is
suggested to give the task a low priority to ensure it does not adversely effect
the performance of other TCP/IP stack activity. */
#define configUDP_LOGGING_TASK_PRIORITY   	( tskIDLE_PRIORITY + 1 )

/* The UDP port to which the UDP logging facility sends messages. */
#define configUDP_LOGGING_PORT_LOCAL	1499
#define configUDP_LOGGING_PORT_REMOTE	1500

/* Here it is possible to define the target IP address to send all UDP
logging as e.g.:

#define configUDP_LOGGING_ADDR0	192
#define configUDP_LOGGING_ADDR1	168
#define configUDP_LOGGING_ADDR2	0
#define configUDP_LOGGING_ADDR3	3

If not defined, the local broadcast address will be used, e.g. 192.168.0.255
*/
#define configUDP_LOGGING_ADDR0	192
#define configUDP_LOGGING_ADDR1	168
#define configUDP_LOGGING_ADDR2	0
#define configUDP_LOGGING_ADDR3	100



/* The trace macros are used to keep a count of seconds. */
extern uint32_t ulSeconds, ulMsec;

#define traceINCREASE_TICK_COUNT( xTicksToJump ) \
	{ \
		ulMsec += xTicksToJump; \
		if( ulMsec >= 1000 ) \
		{ \
			ulSeconds += ( ulMsec / 1000ul ); \
			ulMsec = ( ulMsec % 1000ul ); \
		} \
	}

//如果调度器没有被挂起，每次更新 ms 和 s 的值
#define traceTASK_INCREMENT_TICK( xTickCount ) \
	if( uxSchedulerSuspended == ( UBaseType_t ) pdFALSE ) \
	{ \
		if( ++ulMsec >= 1000 ) \
		{ \
			ulMsec = 0; \
			ulSeconds++; \
		} \
	}

#endif /* FREERTOS_CONFIG_H */
