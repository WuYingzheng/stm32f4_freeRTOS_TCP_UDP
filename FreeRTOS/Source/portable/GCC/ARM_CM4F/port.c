/*
    FreeRTOS V9.0.0 - Copyright (C) 2016 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>>> AND MODIFIED BY <<<< the FreeRTOS exception.

    ***************************************************************************
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
    ***************************************************************************

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry's de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
    the FAQ page "My application does not run, what could be wrong?".  Have you
    defined configASSERT()?

    http://www.FreeRTOS.org/support - In return for receiving this top quality
    embedded software for free we request you assist our global community by
    participating in the support forum.

    http://www.FreeRTOS.org/training - Investing in training allows your team to
    be as productive as possible as early as possible.  Now you can receive
    FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
    Ltd, and the world's leading authority on the world's leading RTOS.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

    http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

/*-----------------------------------------------------------
 * Implementation of functions defined in portable.h for the ARM CM4F port.
 *----------------------------------------------------------*/

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"

#ifndef __VFP_FP__
	#error This port can only be used when the project options are configured to enable hardware floating point support.
#endif

#ifndef configSYSTICK_CLOCK_HZ
	#define configSYSTICK_CLOCK_HZ configCPU_CLOCK_HZ
	/* Ensure the SysTick is clocked at the same frequency as the core. */
	#define portNVIC_SYSTICK_CLK_BIT	( 1UL << 2UL )
#else
	/* The way the SysTick is clocked is not modified in case it is not the same
	as the core. */
	#define portNVIC_SYSTICK_CLK_BIT	( 0 )
#endif

/* Constants required to manipulate the core.  Registers first... */
#define portNVIC_SYSTICK_CTRL_REG			( * ( ( volatile uint32_t * ) 0xe000e010 ) )
#define portNVIC_SYSTICK_LOAD_REG			( * ( ( volatile uint32_t * ) 0xe000e014 ) )
#define portNVIC_SYSTICK_CURRENT_VALUE_REG	( * ( ( volatile uint32_t * ) 0xe000e018 ) )
#define portNVIC_SYSPRI2_REG				( * ( ( volatile uint32_t * ) 0xe000ed20 ) )
/* ...then bits in the registers. */
#define portNVIC_SYSTICK_INT_BIT			( 1UL << 1UL )
#define portNVIC_SYSTICK_ENABLE_BIT			( 1UL << 0UL )
#define portNVIC_SYSTICK_COUNT_FLAG_BIT		( 1UL << 16UL )
#define portNVIC_PENDSVCLEAR_BIT 			( 1UL << 27UL )
#define portNVIC_PEND_SYSTICK_CLEAR_BIT		( 1UL << 25UL )

/* Constants used to detect a Cortex-M7 r0p1 core, which should use the ARM_CM7
r0p1 port. */
#define portCPUID							( * ( ( volatile uint32_t * ) 0xE000ed00 ) )
#define portCORTEX_M7_r0p1_ID				( 0x410FC271UL )
#define portCORTEX_M7_r0p0_ID				( 0x410FC270UL )

#define portNVIC_PENDSV_PRI					( ( ( uint32_t ) configKERNEL_INTERRUPT_PRIORITY ) << 16UL )
#define portNVIC_SYSTICK_PRI				( ( ( uint32_t ) configKERNEL_INTERRUPT_PRIORITY ) << 24UL )

/* Constants required to check the validity of an interrupt priority. */
#define portFIRST_USER_INTERRUPT_NUMBER		( 16 )
#define portNVIC_IP_REGISTERS_OFFSET_16 	( 0xE000E3F0 )
#define portAIRCR_REG						( * ( ( volatile uint32_t * ) 0xE000ED0C ) )
#define portMAX_8_BIT_VALUE					( ( uint8_t ) 0xff )
#define portTOP_BIT_OF_BYTE					( ( uint8_t ) 0x80 )
#define portMAX_PRIGROUP_BITS				( ( uint8_t ) 7 )
#define portPRIORITY_GROUP_MASK				( 0x07UL << 8UL )
#define portPRIGROUP_SHIFT					( 8UL )

/* Masks off all bits but the VECTACTIVE bits in the ICSR register. */
#define portVECTACTIVE_MASK					( 0xFFUL )

/* Constants required to manipulate the VFP. */
#define portFPCCR							( ( volatile uint32_t * ) 0xe000ef34 ) /* Floating point context control register. */
#define portASPEN_AND_LSPEN_BITS			( 0x3UL << 30UL )

/* Constants required to set up the initial stack. */
#define portINITIAL_XPSR					( 0x01000000 )
#define portINITIAL_EXEC_RETURN				( 0xfffffffd )

/* The systick is a 24-bit counter. */
#define portMAX_24_BIT_NUMBER				( 0xffffffUL )

/* For strict compliance with the Cortex-M spec the task start address should
have bit-0 clear, as it is loaded into the PC on exit from an ISR. */
#define portSTART_ADDRESS_MASK		( ( StackType_t ) 0xfffffffeUL )

/* A fiddle factor to estimate the number of SysTick counts that would have
occurred while the SysTick counter is stopped during tickless idle
calculations. */
#define portMISSED_COUNTS_FACTOR			( 45UL )

/* Let the user override the pre-loading of the initial LR with the address of
prvTaskExitError() in case it messes up unwinding of the stack in the
debugger. */
#ifdef configTASK_RETURN_ADDRESS
	#define portTASK_RETURN_ADDRESS	configTASK_RETURN_ADDRESS
#else
	#define portTASK_RETURN_ADDRESS	prvTaskExitError
#endif

/* Each task maintains its own interrupt status in the critical nesting variable.
  该变量在 xPortStartScheduler() 被初始化为0，每进入一次临界区++，每退出一次临界区--，减到0时开启中断
* 内核的临界嵌套变量；如果配置了任务临界嵌套变量，每个任务会维护自己的临界嵌套变量
*/
static UBaseType_t uxCriticalNesting = 0xaaaaaaaa;

/*
 * Setup the timer to generate the tick interrupts.  The implementation in this
 * file is weak to allow application writers to change the timer used to
 * generate the tick interrupt.
 */
void vPortSetupTimerInterrupt( void );

/*
 * Exception handlers.
 */
void xPortPendSVHandler( void ) __attribute__ (( naked ));
void xPortSysTickHandler( void );
void vPortSVCHandler( void ) __attribute__ (( naked ));

/*
 * Start first task is a separate function so it can be tested in isolation.
 */
static void prvPortStartFirstTask( void ) __attribute__ (( naked ));

/*
 * Function to enable the VFP.
 */
static void vPortEnableVFP( void ) __attribute__ (( naked ));

/*
 * Used to catch tasks that attempt to return from their implementing function.
 */
static void prvTaskExitError( void );

/*-----------------------------------------------------------*/

/*
 * The number of SysTick increments that make up one tick period.
 */
#if configUSE_TICKLESS_IDLE == 1
	static uint32_t ulTimerCountsForOneTick = 0;
#endif /* configUSE_TICKLESS_IDLE */

/*
 * The maximum number of tick periods that can be suppressed is limited by the
 * 24 bit resolution of the SysTick timer.
 */
#if configUSE_TICKLESS_IDLE == 1
	static uint32_t xMaximumPossibleSuppressedTicks = 0;
#endif /* configUSE_TICKLESS_IDLE */

/*
 * Compensate for the CPU cycles that pass while the SysTick is stopped (low
 * power functionality only.
 */
#if configUSE_TICKLESS_IDLE == 1
	static uint32_t ulStoppedTimerCompensation = 0;
#endif /* configUSE_TICKLESS_IDLE */

/*
 * Used by the portASSERT_IF_INTERRUPT_PRIORITY_INVALID() macro to ensure
 * FreeRTOS API functions are not called from interrupts that have been assigned
 * a priority above configMAX_SYSCALL_INTERRUPT_PRIORITY.
 */
#if ( configASSERT_DEFINED == 1 )
	 static uint8_t ucMaxSysCallPriority = 0;
	 static uint32_t ulMaxPRIGROUPValue = 0;
	 static const volatile uint8_t * const pcInterruptPriorityRegisters = ( const volatile uint8_t * const ) portNVIC_IP_REGISTERS_OFFSET_16;
#endif /* configASSERT_DEFINED */

/*-----------------------------------------------------------*/

/*
 * 调用函数 prvInitialiseNewTask()
 *
 * 作用：给一个新的任务设置栈，然后才能把他置入调度器的控制
 * StackType_t 无符号32位整型
 */
StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack, TaskFunction_t pxCode, void *pvParameters )
{
	/* Simulate the stack frame as it would be created by a context switch
	interrupt.
	在中断上下文切换时，模拟堆栈帧
	*/

	/* Offset added to account for the way the MCU uses the stack on entry/exit
	of interrupts, and to ensure alignment.
	增加偏移量以说明MCU进出中断的方式，并且确保对齐
	*/
	pxTopOfStack--;

	//关于这个寄存器参考 https://blog.csdn.net/u010893262/article/details/52808639
	*pxTopOfStack = portINITIAL_XPSR;	/* 程序状态寄存器 xPSR */
	pxTopOfStack--;
	*pxTopOfStack = ( ( StackType_t ) pxCode ) & portSTART_ADDRESS_MASK;	/* PC */
	pxTopOfStack--;
	*pxTopOfStack = ( StackType_t ) portTASK_RETURN_ADDRESS;	/* LR; portTASK_RETURN_ADDRESS被配置为NULL*/

	/* Save code space by skipping register initialisation. */
	pxTopOfStack -= 5;	/* R12, R3, R2 and R1. */
	*pxTopOfStack = ( StackType_t ) pvParameters;	/* 参数传递 R0 */

	/* A save method is being used that requires each task to maintain its own exec return value. */
	//用于保存返回值...不太清楚
	pxTopOfStack--;
	*pxTopOfStack = portINITIAL_EXEC_RETURN;

	pxTopOfStack -= 8;	/* R11, R10, R9, R8, R7, R6, R5 and R4. */

	return pxTopOfStack;
}
/*-----------------------------------------------------------*/
/*
 * 调用者 xPortStartScheduler()
 * 为了阻止任务非法返回，设置无线循环，捕捉错误
 */
static void prvTaskExitError( void )
{
	/*
	  A function that implements a task must not exit or attempt to return to
	its caller as there is nothing to return to.  If a task wants to exit it
	should instead call vTaskDelete( NULL ).

	Artificially force an assert() to be triggered if configASSERT() is
	defined, then stop here so application writers can catch the error. */
	configASSERT( uxCriticalNesting == ~0UL );
	portDISABLE_INTERRUPTS();
	for( ;; );
}
/*-----------------------------------------------------------*/
/*
 * 启动第一个任务，触发 SVC 异常后，异常处理函数中直接执行现场恢复， 把 pxCurrentTCB "恢复"到运行状态
 */
void vPortSVCHandler( void )
{
	__asm volatile (
					"	ldr	r3, pxCurrentTCBConst2		\n" /* Restore the context. 取 pxCurrentTCB 的地址*/
					"	ldr r1, [r3]					\n" /* Use pxCurrentTCBConst to get the pxCurrentTCB address. 取出 pxCurrentTCB 的值 ： TCB 地址 */
					"	ldr r0, [r1]					\n" /* The first item in pxCurrentTCB is the task top of stack. 取出 TCB 第一项 ： 任务的栈顶*/
					/* Pop the registers that are not automatically saved on exception entry and the critical nesting count.
					 * 恢复寄存器数据，栈中的数据先弹出到r4，每次传送后r0中的地址增加4*/
					"	ldmia r0!, {r4-r11, r14}		\n"
					"	msr psp, r0						\n" /* Restore the task stack pointer. 设置线程栈指针*/
					"	isb								\n" /*流水线清洗*/
					"	mov r0, #0 						\n" /*r0寄存器载入立即数0*/
					"	msr	basepri, r0					\n" //该寄存器最多有9位（由表达优先级的位数决定）。定义了被屏蔽优先级的阈值。当它被设置为某个值后，所有优先级号大于等于此值的中断都被关。
			                                                //若设置成0，则不关断任何中断，0为默认值。
					"	bx r14							\n" //跳转到 r14
					"									\n"
					"	.align 4						\n"
			//label: .word value: places the 4-byte value at the address assigned (by the linker) to label.
					"pxCurrentTCBConst2: .word pxCurrentTCB				\n"
				);
}
/*-----------------------------------------------------------*/
/*
 * 这个过程
 *   重新设置了栈指针;
 *   将控制寄存器清零；
 *   触发svc异常，启动第一个任务
 */
static void prvPortStartFirstTask( void )
{
	/* Start the first task.  This also clears the bit that indicates the FPU is
	in use in case the FPU was used before the scheduler was started - which
	would otherwise result in the unnecessary leaving of space in the SVC stack
	for lazy saving of FPU registers. */
	__asm volatile(
					" ldr r0, =0xE000ED08 	\n" /* Use the NVIC offset register to locate the stack.向量表偏移寄存器地址 */
					" ldr r0, [r0] 			\n" /*取向量表地址*/
					" ldr r0, [r0] 			\n" /*取 MSP 初始值*/
					" msr msp, r0			\n" /* Set the msp back to the start of the stack. */
					" mov r0, #0			\n" /* Clear the bit that indicates the FPU is in use, see comment above. */
					" msr control, r0		\n" //控制寄存器清零
					" cpsie i				\n" /* Globally enable interrupts. */
					" cpsie f				\n" //打开异常
					" dsb					\n"	//数据同步
					" isb					\n" //指令同步隔离
					" svc 0					\n" /* System call to start first task. 触发异常，启用第一个任务*/
					" nop					\n"
				);
}
/*-----------------------------------------------------------*/

/*
 * See header file for description.
 */
BaseType_t xPortStartScheduler( void )
{
	/* configMAX_SYSCALL_INTERRUPT_PRIORITY must not be set to 0.
	See http://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html */
	configASSERT( configMAX_SYSCALL_INTERRUPT_PRIORITY );

	/* This port can be used on all revisions of the Cortex-M7 core other than
	the r0p1 parts.  r0p1 parts should use the port from the
	/source/portable/GCC/ARM_CM7/r0p1 directory. */
	configASSERT( portCPUID != portCORTEX_M7_r0p1_ID );
	configASSERT( portCPUID != portCORTEX_M7_r0p0_ID );

	#if( configASSERT_DEFINED == 1 )
	{
		volatile uint32_t ulOriginalPriority;
		volatile uint8_t * const pucFirstUserPriorityRegister = ( volatile uint8_t * const ) ( portNVIC_IP_REGISTERS_OFFSET_16 + portFIRST_USER_INTERRUPT_NUMBER );
		volatile uint8_t ucMaxPriorityValue;

		/* Determine the maximum priority from which ISR safe FreeRTOS API
		functions can be called.  ISR safe functions are those that end in
		"FromISR".  FreeRTOS maintains separate thread and ISR API functions to
		ensure interrupt entry is as fast and simple as possible.

		Save the interrupt priority value that is about to be clobbered. */
		ulOriginalPriority = *pucFirstUserPriorityRegister;

		/* Determine the number of priority bits available.  First write to all possible bits. */
		//判断处理器支持的优先级位数，先全部写入11,
		*pucFirstUserPriorityRegister = portMAX_8_BIT_VALUE;

		/* Read the value back to see how many bits stuck. */
		//重新读回，不能设置的位依然为0
		ucMaxPriorityValue = *pucFirstUserPriorityRegister;

		/* Use the same mask on the maximum system call priority. */
		//确保用户设置的优先级不会超出范围
		ucMaxSysCallPriority = configMAX_SYSCALL_INTERRUPT_PRIORITY & ucMaxPriorityValue;

		/* Calculate the maximum acceptable priority group value for the number of bits read back. */
		//计算最大的组优先级，将 ucMaxPriorityValue 向左进行移位计算
		ulMaxPRIGROUPValue = portMAX_PRIGROUP_BITS;
		while( ( ucMaxPriorityValue & portTOP_BIT_OF_BYTE ) == portTOP_BIT_OF_BYTE )
		{
			ulMaxPRIGROUPValue--;
			ucMaxPriorityValue <<= ( uint8_t ) 0x01;
		}

		#ifdef __NVIC_PRIO_BITS
		{
			/* Check the CMSIS configuration that defines the number of
			priority bits matches the number of priority bits actually queried
			from the hardware. */
			configASSERT( ( portMAX_PRIGROUP_BITS - ulMaxPRIGROUPValue ) == __NVIC_PRIO_BITS );
		}
		#endif

		#ifdef configPRIO_BITS
		{
			/* Check the FreeRTOS configuration that defines the number of
			priority bits matches the number of priority bits actually queried
			from the hardware. */
			configASSERT( ( portMAX_PRIGROUP_BITS - ulMaxPRIGROUPValue ) == configPRIO_BITS );
		}
		#endif

		/* Shift the priority group value back to its position within the AIRCR register. */
		ulMaxPRIGROUPValue <<= portPRIGROUP_SHIFT;
		ulMaxPRIGROUPValue &= portPRIORITY_GROUP_MASK;

		// Restore the clobbered interrupt priority register to its original value.
		// 恢复优先级配置寄存器
		*pucFirstUserPriorityRegister = ulOriginalPriority;
	}
	#endif /* conifgASSERT_DEFINED */

	// Make PendSV and SysTick the lowest priority interrupts.
	// 保证系统会话切换不会阻塞系统其他中断的响应
	portNVIC_SYSPRI2_REG |= portNVIC_PENDSV_PRI;
	portNVIC_SYSPRI2_REG |= portNVIC_SYSTICK_PRI;

	/* Start the timer that generates the tick ISR.  Interrupts are disabled
	here already. */
	vPortSetupTimerInterrupt();

	/* Initialise the critical nesting count ready for the first task. */
	uxCriticalNesting = 0;

	/* Ensure the VFP is enabled - it should be anyway. */
	vPortEnableVFP();

	/* Lazy save always. */
	*( portFPCCR ) |= portASPEN_AND_LSPEN_BITS;

	/* Start the first task. */
	prvPortStartFirstTask();

	/* Should never get here as the tasks will now be executing!  Call the task
	exit error function to prevent compiler warnings about a static function
	not being called in the case that the application writer overrides this
	functionality by defining configTASK_RETURN_ADDRESS. */
	prvTaskExitError();

	/* Should not get here! */
	return 0;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
	/* Not implemented in ports where there is nothing to return to.
	Artificially force an assert. */
	configASSERT( uxCriticalNesting == 1000UL );
}
/*-----------------------------------------------------------*/

void vPortEnterCritical( void )
{
	portDISABLE_INTERRUPTS();
	uxCriticalNesting++;

	/* This is not the interrupt safe version of the enter critical function so
	assert() if it is being called from an interrupt context.  Only API
	functions that end in "FromISR" can be used in an interrupt.  Only assert if
	the critical nesting count is 1 to protect against recursive calls if the
	assert function also uses a critical section. */
	if( uxCriticalNesting == 1 ) //如果该函数在中断中被调用，之前没有进入临界嵌套区，那么之前 uxCriticalNesting==0
	{
		configASSERT( ( portNVIC_INT_CTRL_REG & portVECTACTIVE_MASK ) == 0 ); //不应该有中断激活标志
	}
}
/*-----------------------------------------------------------*/

void vPortExitCritical( void )
{
	configASSERT( uxCriticalNesting );
	uxCriticalNesting--;
	if( uxCriticalNesting == 0 )
	{
		portENABLE_INTERRUPTS();
	}
}
/*-----------------------------------------------------------*/
/*用于切换上下文，通过悬起PendSV异常，系统会直到其它所有ISR都处理完成，才执行该异常服务程序
 * ：保存当前任务现场，选择切换的任务，进行任务切换，退出异常会线程任务运行新任务。
 *  系统进入异常处理程序的时候，使用的是主堆栈指针 MSP， 而一般情况下运行任务使用的线程模式使用的是进程堆栈指针 PSP
 *  对应这两个指针，系统有两种堆栈，系统内核和异常程序处理使用的是主堆栈，MSP 指向其栈顶。而对应而不同任务，我们在创
 *  建时为其分配了空间，作为该任务的堆栈，在该任务运行时，由系统设置进程堆栈 PSP 指向该栈顶。
 *  进入中断服务例程时，硬件会自动将R3——R0,R12.LR，PC，XPSR入栈。
*/
void xPortPendSVHandler( void )
{
	/* This is a naked function. */

	__asm volatile
	(
	"	mrs r0, psp							\n" // 取出当前任务的栈顶指针 也就是 psp -> R0
	"	isb									\n" // 指令同步隔离
	"										\n"
	"	ldr	r3, pxCurrentTCBConst			\n" /* Get the location of the current TCB. */
	"	ldr	r2, [r3]						\n" // TCB的第一个字段是栈顶指针
	"										\n"
	"	tst r14, #0x10						\n" /* Is the task using the FPU context?  If so, push high vfp registers. */
	"	it eq								\n"
	"	vstmdbeq r0!, {s16-s31}				\n"
	"										\n"
	"	stmdb r0!, {r4-r11, r14}			\n" /* r0先-4,再压栈，先压r14，循环，退出中断前，硬件自动将xPSR、PC、LR、R12、R0-R3压入栈，Save the core registers. */
	"	str r0, [r2]						\n" /* 把栈顶写入 pxCurrentTCB ，Save the new top of stack into the first member of the TCB. */
	"										\n"
	"	stmdb sp!, {r3}						\n" //将R3压入主栈中
	"	mov r0, %0 							\n" //进入临界区域
	"	msr basepri, r0						\n" //...
	"	dsb									\n" //数据同步隔离
	"	isb									\n" //指令同步隔离
	"	bl vTaskSwitchContext				\n" //调用函数切换上下文，通过使变量pxCurrentTCB指向新的任务来实现任务切换
	"	mov r0, #0							\n" //退出临界区
	"	msr basepri, r0						\n" //...
	"	ldmia sp!, {r3}						\n" //恢复R3寄存器的值
	"										\n"
	"	ldr r1, [r3]						\n" /* The first item in pxCurrentTCB is the task top of stack. */
	"	ldr r0, [r1]						\n" //载入新任务的栈地址
	"										\n"
	"	ldmia r0!, {r4-r11, r14}			\n" /* 这些寄存器不会主动出栈，需要手动出栈，Pop the core registers. */
	"										\n"
	"	tst r14, #0x10						\n" /* Is the task using the FPU context?  If so, pop the high vfp registers too. */
	"	it eq								\n"
	"	vldmiaeq r0!, {s16-s31}				\n"
	"										\n"
	"	msr psp, r0							\n"
	"	isb									\n" //指令隔离
	"										\n"
	#ifdef WORKAROUND_PMU_CM001 /* XMC4000 specific errata workaround. */
		#if WORKAROUND_PMU_CM001 == 1
	"			push { r14 }				\n"
	"			pop { pc }					\n"
		#endif
	#endif
	"										\n"
	/* 异常发生时,R14中保存异常返回标志,包括返回后进入线程模式还是处理器模式、使用PSP堆栈指针还是MSP堆栈指针，
	 * 当调用 bx r14指令后，硬件会知道要从异常返回，然后出栈，这个时候堆栈指针PSP已经指向了新任务堆栈的正确位置，
	 * 当新任务的运行地址被出栈到PC寄存器后，新的任务也会被执行。
	 */
	"	bx r14								\n" //通知硬件从异常返回，硬件会自动将R3——R0,R12.LR，PC，XPSR出栈
	"										\n"
	"	.align 4							\n"
	"pxCurrentTCBConst: .word pxCurrentTCB	\n"
	::"i"(configMAX_SYSCALL_INTERRUPT_PRIORITY)
	);
}
/*-----------------------------------------------------------*/
//调用 xPortSysTickHandler(), 根据返回值判断是否需要触发一个 pendsv 异常，请求上下文切换
void xPortSysTickHandler( void )
{
	/* The SysTick runs at the lowest interrupt priority, so when this interrupt
	executes all interrupts must be unmasked.  There is therefore no need to
	save and then restore the interrupt mask value as its value is already
	known. */
	portDISABLE_INTERRUPTS();
	{
		/* Increment the RTOS tick. */
		if( xTaskIncrementTick() != pdFALSE )
		{
			/* A context switch is required.  Context switching is performed in
			the PendSV interrupt.  Pend the PendSV interrupt. */
			portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;
		}
	}
	portENABLE_INTERRUPTS();
}
/*-----------------------------------------------------------*/

#if configUSE_TICKLESS_IDLE == 1

	__attribute__((weak)) void vPortSuppressTicksAndSleep( TickType_t xExpectedIdleTime )
	{
	uint32_t ulReloadValue, ulCompleteTickPeriods, ulCompletedSysTickDecrements, ulSysTickCTRL;
	TickType_t xModifiableIdleTime;

		/* Make sure the SysTick reload value does not overflow the counter. */
		if( xExpectedIdleTime > xMaximumPossibleSuppressedTicks )
		{
			xExpectedIdleTime = xMaximumPossibleSuppressedTicks;
		}

		/* Stop the SysTick momentarily.  The time the SysTick is stopped for
		is accounted for as best it can be, but using the tickless mode will
		inevitably result in some tiny drift of the time maintained by the
		kernel with respect to calendar time. */
		portNVIC_SYSTICK_CTRL_REG &= ~portNVIC_SYSTICK_ENABLE_BIT;

		/* Calculate the reload value required to wait xExpectedIdleTime
		tick periods.  -1 is used because this code will execute part way
		through one of the tick periods. */
		ulReloadValue = portNVIC_SYSTICK_CURRENT_VALUE_REG + ( ulTimerCountsForOneTick * ( xExpectedIdleTime - 1UL ) );
		if( ulReloadValue > ulStoppedTimerCompensation )
		{
			ulReloadValue -= ulStoppedTimerCompensation;
		}

		/* Enter a critical section but don't use the taskENTER_CRITICAL()
		method as that will mask interrupts that should exit sleep mode. */
		__asm volatile( "cpsid i" );
		__asm volatile( "dsb" );
		__asm volatile( "isb" );

		/* If a context switch is pending or a task is waiting for the scheduler
		to be unsuspended then abandon the low power entry. */
		if( eTaskConfirmSleepModeStatus() == eAbortSleep )
		{
			/* Restart from whatever is left in the count register to complete
			this tick period. */
			portNVIC_SYSTICK_LOAD_REG = portNVIC_SYSTICK_CURRENT_VALUE_REG;

			/* Restart SysTick. */
			portNVIC_SYSTICK_CTRL_REG |= portNVIC_SYSTICK_ENABLE_BIT;

			/* Reset the reload register to the value required for normal tick
			periods. */
			portNVIC_SYSTICK_LOAD_REG = ulTimerCountsForOneTick - 1UL;

			/* Re-enable interrupts - see comments above the cpsid instruction()
			above. */
			__asm volatile( "cpsie i" );
		}
		else
		{
			/* Set the new reload value. */
			portNVIC_SYSTICK_LOAD_REG = ulReloadValue;

			/* Clear the SysTick count flag and set the count value back to
			zero. */
			portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;

			/* Restart SysTick. */
			portNVIC_SYSTICK_CTRL_REG |= portNVIC_SYSTICK_ENABLE_BIT;

			/* Sleep until something happens.  configPRE_SLEEP_PROCESSING() can
			set its parameter to 0 to indicate that its implementation contains
			its own wait for interrupt or wait for event instruction, and so wfi
			should not be executed again.  However, the original expected idle
			time variable must remain unmodified, so a copy is taken. */
			xModifiableIdleTime = xExpectedIdleTime;
			configPRE_SLEEP_PROCESSING( xModifiableIdleTime );
			if( xModifiableIdleTime > 0 )
			{
				__asm volatile( "dsb" );
				__asm volatile( "wfi" );
				__asm volatile( "isb" );
			}
			configPOST_SLEEP_PROCESSING( xExpectedIdleTime );

			/* Stop SysTick.  Again, the time the SysTick is stopped for is
			accounted for as best it can be, but using the tickless mode will
			inevitably result in some tiny drift of the time maintained by the
			kernel with respect to calendar time. */
			ulSysTickCTRL = portNVIC_SYSTICK_CTRL_REG;
			portNVIC_SYSTICK_CTRL_REG = ( ulSysTickCTRL & ~portNVIC_SYSTICK_ENABLE_BIT );

			/* Re-enable interrupts - see comments above the cpsid instruction()
			above. */
			__asm volatile( "cpsie i" );

			if( ( ulSysTickCTRL & portNVIC_SYSTICK_COUNT_FLAG_BIT ) != 0 )
			{
				uint32_t ulCalculatedLoadValue;

				/* The tick interrupt has already executed, and the SysTick
				count reloaded with ulReloadValue.  Reset the
				portNVIC_SYSTICK_LOAD_REG with whatever remains of this tick
				period. */
				ulCalculatedLoadValue = ( ulTimerCountsForOneTick - 1UL ) - ( ulReloadValue - portNVIC_SYSTICK_CURRENT_VALUE_REG );

				/* Don't allow a tiny value, or values that have somehow
				underflowed because the post sleep hook did something
				that took too long. */
				if( ( ulCalculatedLoadValue < ulStoppedTimerCompensation ) || ( ulCalculatedLoadValue > ulTimerCountsForOneTick ) )
				{
					ulCalculatedLoadValue = ( ulTimerCountsForOneTick - 1UL );
				}

				portNVIC_SYSTICK_LOAD_REG = ulCalculatedLoadValue;

				/* The tick interrupt handler will already have pended the tick
				processing in the kernel.  As the pending tick will be
				processed as soon as this function exits, the tick value
				maintained by the tick is stepped forward by one less than the
				time spent waiting. */
				ulCompleteTickPeriods = xExpectedIdleTime - 1UL;
			}
			else
			{
				/* Something other than the tick interrupt ended the sleep.
				Work out how long the sleep lasted rounded to complete tick
				periods (not the ulReload value which accounted for part
				ticks). */
				ulCompletedSysTickDecrements = ( xExpectedIdleTime * ulTimerCountsForOneTick ) - portNVIC_SYSTICK_CURRENT_VALUE_REG;

				/* How many complete tick periods passed while the processor
				was waiting? */
				ulCompleteTickPeriods = ulCompletedSysTickDecrements / ulTimerCountsForOneTick;

				/* The reload value is set to whatever fraction of a single tick
				period remains. */
				portNVIC_SYSTICK_LOAD_REG = ( ( ulCompleteTickPeriods + 1UL ) * ulTimerCountsForOneTick ) - ulCompletedSysTickDecrements;
			}

			/* Restart SysTick so it runs from portNVIC_SYSTICK_LOAD_REG
			again, then set portNVIC_SYSTICK_LOAD_REG back to its standard
			value.  The critical section is used to ensure the tick interrupt
			can only execute once in the case that the reload register is near
			zero. */
			portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;
			portENTER_CRITICAL();
			{
				portNVIC_SYSTICK_CTRL_REG |= portNVIC_SYSTICK_ENABLE_BIT;
				vTaskStepTick( ulCompleteTickPeriods );
				portNVIC_SYSTICK_LOAD_REG = ulTimerCountsForOneTick - 1UL;
			}
			portEXIT_CRITICAL();
		}
	}

#endif /* #if configUSE_TICKLESS_IDLE */
/*-----------------------------------------------------------*/

/*
 * Setup the systick timer to generate the tick interrupts at the required
 * frequency.
 */
__attribute__(( weak )) void vPortSetupTimerInterrupt( void )
{
	/* Calculate the constants required to configure the tick interrupt. */
	#if configUSE_TICKLESS_IDLE == 1
	{
		ulTimerCountsForOneTick = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ );
		xMaximumPossibleSuppressedTicks = portMAX_24_BIT_NUMBER / ulTimerCountsForOneTick;
		ulStoppedTimerCompensation = portMISSED_COUNTS_FACTOR / ( configCPU_CLOCK_HZ / configSYSTICK_CLOCK_HZ );
	}
	#endif /* configUSE_TICKLESS_IDLE */

	/* Stop and clear the SysTick. */
	portNVIC_SYSTICK_CTRL_REG = 0UL;
	portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;

	/* Configure SysTick to interrupt at the requested rate. */
	portNVIC_SYSTICK_LOAD_REG = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ ) - 1UL;
	portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT );
}
/*-----------------------------------------------------------*/

/* This is a naked function. */
static void vPortEnableVFP( void )
{
	__asm volatile
	(
		"	ldr.w r0, =0xE000ED88		\n" /* The FPU enable bits are in the CPACR. */
		"	ldr r1, [r0]				\n"
		"								\n"
		"	orr r1, r1, #( 0xf << 20 )	\n" /* Enable CP10 and CP11 coprocessors, then save back. */
		"	str r1, [r0]				\n"
		"	bx r14						"
	);
}
/*-----------------------------------------------------------*/

#if( configASSERT_DEFINED == 1 )

	void vPortValidateInterruptPriority( void )
	{
	uint32_t ulCurrentInterrupt;
	uint8_t ucCurrentPriority;

		/* Obtain the number of the currently executing interrupt. */
		__asm volatile( "mrs %0, ipsr" : "=r"( ulCurrentInterrupt ) );

		/* Is the interrupt number a user defined interrupt? */
		if( ulCurrentInterrupt >= portFIRST_USER_INTERRUPT_NUMBER )
		{
			/* Look up the interrupt's priority. */
			ucCurrentPriority = pcInterruptPriorityRegisters[ ulCurrentInterrupt ];

			/* The following assertion will fail if a service routine (ISR) for
			an interrupt that has been assigned a priority above
			configMAX_SYSCALL_INTERRUPT_PRIORITY calls an ISR safe FreeRTOS API
			function.  ISR safe FreeRTOS API functions must *only* be called
			from interrupts that have been assigned a priority at or below
			configMAX_SYSCALL_INTERRUPT_PRIORITY.

			Numerically low interrupt priority numbers represent logically high
			interrupt priorities, therefore the priority of the interrupt must
			be set to a value equal to or numerically *higher* than
			configMAX_SYSCALL_INTERRUPT_PRIORITY.

			Interrupts that	use the FreeRTOS API must not be left at their
			default priority of	zero as that is the highest possible priority,
			which is guaranteed to be above configMAX_SYSCALL_INTERRUPT_PRIORITY,
			and	therefore also guaranteed to be invalid.

			FreeRTOS maintains separate thread and ISR API functions to ensure
			interrupt entry is as fast and simple as possible.

			The following links provide detailed information:
			http://www.freertos.org/RTOS-Cortex-M3-M4.html
			http://www.freertos.org/FAQHelp.html */
			configASSERT( ucCurrentPriority >= ucMaxSysCallPriority );
		}

		/* Priority grouping:  The interrupt controller (NVIC) allows the bits
		that define each interrupt's priority to be split between bits that
		define the interrupt's pre-emption priority bits and bits that define
		the interrupt's sub-priority.  For simplicity all bits must be defined
		to be pre-emption priority bits.  The following assertion will fail if
		this is not the case (if some bits represent a sub-priority).

		If the application only uses CMSIS libraries for interrupt
		configuration then the correct setting can be achieved on all Cortex-M
		devices by calling NVIC_SetPriorityGrouping( 0 ); before starting the
		scheduler.  Note however that some vendor specific peripheral libraries
		assume a non-zero priority group setting, in which cases using a value
		of zero will result in unpredicable behaviour. */
		configASSERT( ( portAIRCR_REG & portPRIORITY_GROUP_MASK ) <= ulMaxPRIGROUPValue );
	}

#endif /* configASSERT_DEFINED */


