/*
*****************************************************************************
**
**  File        : stm32_flash.ld
**
**  Abstract    : 原来的开发板 stm32f407vgt   我使用的开发板 stm32f407zgt
**                v：100引脚
**                Z: 144引脚   G: 1M flash  T: LQFP封装
**                
**                在管脚数量上有差别，其它没有任何差别，所以编程方面没有任何差别
**
**          		  Linker script for STM32F407IG Device with
**                1024KByte FLASH, 128KByte RAM
**
**                Set heap size, stack size and stack location according
**                to application requirements.
**
**                Set memory bank area and size if external memory is used.
**
**  Target      : STMicroelectronics STM32
**
**  Environment : Atollic TrueSTUDIO(R)
**
**  Distribution: The file is distributed "as is", without any warranty
**                of any kind.
**
**  (c)Copyright Atollic AB.
**  You may use this file as-is or modify it according to
 the needs of your
**  project. This file may only be built (assembled or compiled and linked)
**  using the Atollic TrueSTUDIO(R) product. The use of this file together
**  with other tools than Atollic TrueSTUDIO(R) is not permitted.
**
*****************************************************************************
*/

/* Entry Point 程序的入口地址*/
ENTRY(Reset_Handler)

/* Highest address of the user mode stack 栈的最高处地址，即栈底*/
_estack = 0x20020000;    /* end of 128K RAM */

/* Generate a link error if heap and stack don't fit into RAM 
 * 最小的堆大小和栈大小
 *
 */
_Min_Heap_Size = 0;      /* required amount of heap  */
_Min_Stack_Size = 0x400; /* required amount of stack */

/* Specify the memory areas 内存的布局
 * Flash : 开始地址，长度1M
 * RAM   : 开始地址， 长度128K
 * MEMORY_B1: 保存芯片信息的配置
 * CCMRAM: CCMRAM,内核能够直接访问的一段内存 64K
 * 当代码处于CCM，数据保存在普通ram中时，Cotex-m4内核处于最优的哈佛配置
 */
MEMORY
{
  FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 1024K
  RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 128K
  MEMORY_B1 (rx)  : ORIGIN = 0x60000000, LENGTH = 0K
  CCMRAM (rw)     : ORIGIN = 0x10000000, LENGTH = 64K
}

/* Define output sections */
SECTIONS
{
  /* The startup code goes first into FLASH */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector)) /* Startup code */
    . = ALIGN(4);
  } >FLASH

  /* The program code and other data goes into FLASH */
  .text :
  {
    . = ALIGN(4);
    *(.text)           /* .text sections (code) */
    *(.text*)          /* .text* sections (code) */
    *(.glue_7)         /* glue arm to thumb code 可能与指令集转换有关 */
    *(.glue_7t)        /* glue thumb to arm code */
    *(.eh_frame)
                       /* 链接时垃圾收集(‘–gc-sections’),会删除一些输出内容 */
    KEEP (*(.init))    /* 把 *.init 段标记为不应该删除的段 */
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;        /* define a global symbols at end of code 标记代码段结束的全局符号*/
  } >FLASH

  /* Constant data goes into FLASH 只读数据段*/
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
    . = ALIGN(4);
  } >FLASH

/* Exception Handling ABI for the ARM Architecture 
 * 参考文档 ELF for the ARM® Architecture 
 * http://infocenter.arm.com/help/topic/com.arm.doc.ihi0044f/IHI0044F_aaelf.pdf
 */
  .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH
  .ARM : {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >FLASH

/* Preinit/init/fini arrays contain pointer for constructors and destructors 
 * functions indeed. These are used notably for constructors and destructors of
 * static objects in C++ but can also be used to run specific functions at program 
 * startup and end in C, by using the eponymous function attributes. If your 
 * program is simple and don't need any constructor to be called, your startup 
 * code can call directly main and skip running constructors. Otherwise the function 
 * pointed to in these arrays need to be called. Whether to copy the array depends 
 * on how they are called: via their LMA or via their VMA.
 * 可能和启动文件相关
 */
  .preinit_array     :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >FLASH
  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >FLASH
  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >FLASH

  /* used by the startup to initialize data 
   * LOADADDR 会强制一个表达式变为绝对地址,标记date段的开始
   */
  _sidata = LOADADDR(.data);

  /* Initialized data sections goes into RAM, load LMA copy after code 
   * 可读写的需要初始化的数据段
   */
  .data : 
  {
    . = ALIGN(4);
    _sdata = .;        /* create a global symbol at data start */
    *(.data)           /* .data sections */
    *(.data*)          /* .data* sections */

    . = ALIGN(4);
    _edata = .;        /* define a global symbol at data end 标记 date段的结束 */
  } >RAM AT> FLASH
  
  _siccmram = LOADADDR(.ccmram);

  /* CCM-RAM section 
  * 
  * IMPORTANT NOTE! 
  * If initialized variables will be placed in this section, 
  * the startup code needs to be modified to copy the init-values.  
  * 只有bootloader拷贝这段代码到 ccm 中时才会有用,可以使用下面的示例:

                movs    r1,#0
                b       LoopCopyDataInit
 
            CopyCCMDataInit:
                  ldr     r3,=_siccmdata
                  ldr     r3,[r3,r1]
                  str     r3,[r0,r1]
                  adds    r1,r1,#4

            LoopCopyCCMDataInit:
                  ldr     r0,=_sccmdata
                  ldr     r3,=_eccmdata
                  adds    r2,r0,r1
                  cmp     r2,r3
                  bcc     CopyCCMDataInit

  * 如果你想在 C语言中指定一相应的代码到这个段中，可以通过下面的方法
  * __attribute__((section(".ccmram-strings"))) const char *helloString = "Hello World";
  * 为了让代码看起来更加简介，可以使用宏定义：
  * #define CCMRAM __attribute__((section(".ccmram")))
  * 然后：
  * CCMRAM const char *helloString = "Hello World"; 
  * 或者...
  * const char *helloString CCMRAM = "Hello World";
  */
  .ccmram :
  {
    . = ALIGN(4);
    _sccmram = .;       /* create a global symbol at ccmram start */
    *(.ccmram)
    *(.ccmram*)
    
    . = ALIGN(4);
    _eccmram = .;       /* create a global symbol at ccmram end */
  } >CCMRAM AT> FLASH

  /* Uninitialized data section 
   * 未初始化的数据段,bootloader会初始化这段数据
   */
  . = ALIGN(4);
  .bss :
  {
    /* This is used by the startup in order to initialize the .bss secion */
    _sbss = .;         /* define a global symbol at bss start */
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)

    . = ALIGN(4);
    _ebss = .;         /* define a global symbol at bss end */
    __bss_end__ = _ebss;
  } >RAM

  /* User_heap_stack section, used to check that there is enough RAM left 
   * 检查用户设置的堆栈空间是否没有超出容量
   */
  ._user_heap_stack :
  {
    . = ALIGN(4);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(4);
  } >RAM

  /* MEMORY_bank1 section, code must be located here explicitly            */
  /* Example: extern int foo(void) __attribute__ ((section (".mb1text"))); */
  .memory_b1_text :
  {
    *(.mb1text)        /* .mb1text sections (code) */
    *(.mb1text*)       /* .mb1text* sections (code)  */
    *(.mb1rodata)      /* read-only data (constants) */
    *(.mb1rodata*)
  } >MEMORY_B1

  /* Remove information from the standard libraries */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
