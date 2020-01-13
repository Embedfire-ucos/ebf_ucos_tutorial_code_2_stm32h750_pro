;
;********************************************************************************************************
;                                                uC/OS-III
;                                          The Real-Time Kernel
;
;
;                           (c) Copyright 2009-2016; Micrium, Inc.; Weston, FL
;                    All rights reserved.  Protected by international copyright laws.
;
;                                              ARMv7-M Port
;
; 文件名    : OS_CPU_A.ASM
; 版本号    : V3.06.00
; 作者      : JJL
;             BAN
;             JBL
;
; CPU 内核  : ARMv7M Cortex-M
; 指令模式  : Thumb-2 ISA
; 开发环境  : MDK(KEIL)
;
; 翻译+注释   : 秉火 fire
; 论坛        : www.firebbs.cn
; 淘宝        : www.fire-stm32.taobao.com
;
; 注意        : (1) 此端口支持 ARM Cortex-M3、Cortex-M4 和 Cortex-M7 架构.
;               (2) 以下硬件浮点单元已经过测试
;                  (a) 单精度: FPv4-SP-D16-M 和 FPv5-SP-D16-M
;                  (b) 双精度: FPv5-D16-M
;********************************************************************************************************
;

;********************************************************************************************************
;                                          公共函数
;********************************************************************************************************

    ; IMPORT的作用是声明标号来自外部文件，跟 C 语言中的 EXTERN 关键字类似
	; 外部引用
    
    IMPORT  OSPrioCur
    IMPORT  OSPrioHighRdy
    IMPORT  OSTCBCurPtr
    IMPORT  OSTCBHighRdyPtr
    IMPORT  OSIntExit
    IMPORT  OSTaskSwHook
    IMPORT  OS_CPU_ExceptStkBase

    ; EXPORT的作用是声明一个标号具有全局属性，可被外部的文件使用
	; 本文件函数声明
    EXPORT  OSStartHighRdy
    EXPORT  OSCtxSw
    EXPORT  OSIntCtxSw
    EXPORT  OS_CPU_PendSVHandler
    
    ; 是否使用硬件浮点
    IF {FPU} != "SoftVFP"
    EXPORT  OS_CPU_FP_Reg_Push
    EXPORT  OS_CPU_FP_Reg_Pop
    ENDIF


;********************************************************************************************************
;                                               常量定义
;********************************************************************************************************

NVIC_INT_CTRL   EQU     0xE000ED04                              ; 中断控制状态寄存器
NVIC_SYSPRI14   EQU     0xE000ED22                              ; 系统优先级寄存器 (优先级14)
NVIC_PENDSV_PRI EQU           0xFF                              ; PendSV的优先级 (最低优先级)
NVIC_PENDSVSET  EQU     0x10000000                              ; 触发PendSV异常的值


;********************************************************************************************************
;                                              代码生成指令
;********************************************************************************************************

    PRESERVE8       ; 8字节对齐
    THUMB           ; THUMB 指令模式

; 用AREA伪指令 汇编一个代码段，名字为CODE（第一个），属性为只读
    AREA CODE, CODE, READONLY


;********************************************************************************************************
;                                   浮点寄存器入栈
;                             void  OS_CPU_FP_Reg_Push (CPU_STK  *stkPtr)
;
;注意 : 1) 此函数保存浮点单元的寄存器S16-S31 。
;
;       2) 伪代码如下:
;          a) 在进程堆栈入栈浮点单元的寄存器S16-S31；
;          b) 更新任务控制器的堆栈 OSTCBCurPtr->StkPtr指针;
;********************************************************************************************************

    IF {FPU} != "SoftVFP"
        
OS_CPU_FP_Reg_Push
    MRS     R1, PSP                                             ; PSP是进程堆栈指针
    CBZ     R1, OS_CPU_FP_nosave                                ; 跳过FPU寄存器第一次保存

    VSTMDB  R0!, {S16-S31}
    LDR     R1, =OSTCBCurPtr
    LDR     R2, [R1]
    STR     R0, [R2]
OS_CPU_FP_nosave
    BX      LR
    
    ENDIF


;********************************************************************************************************
;                                   浮点寄存器出栈
;                             void  OS_CPU_FP_Reg_Pop (CPU_STK  *stkPtr)
;
; Note(s) : 1) 此函数恢复浮点单元的寄存器S16-S31 。
;
;           2) 伪代码如下:
;              a) 恢复新过程堆栈的浮点单元的寄存器S16-S31;
;              b) 更新新进程堆栈的OSTCBHighRdyPtr-> StkPtr指针;
;********************************************************************************************************

    IF {FPU} != "SoftVFP"
    
OS_CPU_FP_Reg_Pop
    VLDMIA  R0!, {S16-S31}
    LDR     R1, =OSTCBHighRdyPtr
    LDR     R2, [R1]
    STR     R0, [R2]
    BX      LR

    ENDIF


;********************************************************************************************************
;                                         开始多任务
;                                      void OSStartHighRdy(void)
;
; 注意:     1) 此函数触发PendSV异常（实质上，导致上下文切换）导致第一个任务开始
;
;           2) 在任务执行期间，PSP用作堆栈指针.
;              当发生异常时，核心将切换到MSP，直到异常返回
;
;           3) OSStartHighRdy() 必须：
;              a) 将PendSV异常优先级设置为最低；
;              b) 初始化进程堆栈指针PSP等于0, 表明这是第一次上下文切换；
;              c) 设置主堆栈指针等于OS_CPU_ExceptStkBase；
;              d) 获取当前的高优先级, OSPrioCur = OSPrioHighRdy；
;              e) 获取当前就绪的线程 TCB, OSTCBCurPtr = OSTCBHighRdyPtr；
;              f) 从TCB获取新进程堆栈指针SP, SP = OSTCBHighRdyPtr->StkPtr；
;              g) 从新的进程堆栈恢复R4--R11和R14寄存器；
;              h) 使能中断 （任务将在启用中断后运行）。
;********************************************************************************************************

OSStartHighRdy
    CPSID   I                                                   ; 关中断，防止中断期间的上下文切换
    MOV32   R0, NVIC_SYSPRI14                                   ; 设置 PendSV 异常优先级
    MOV32   R1, NVIC_PENDSV_PRI
    STRB    R1, [R0]

    MOVS    R0, #0                                              ; 初始化进程堆栈指针PSP等于0,在初始上下文切换时调用
    MSR     PSP, R0

    MOV32   R0, OS_CPU_ExceptStkBase                            ; 设置主堆栈指针等于OS_CPU_ExceptStkBase
    LDR     R1, [R0]
    MSR     MSP, R1

    BL      OSTaskSwHook                                        ; 调用OSTaskSwHook()函数为浮点单元入栈和出栈

    MOV32   R0, OSPrioCur                                       ; OSPrioCur   = OSPrioHighRdy;
    MOV32   R1, OSPrioHighRdy
    LDRB    R2, [R1]
    STRB    R2, [R0]

    MOV32   R0, OSTCBCurPtr                                     ; OSTCBCurPtr = OSTCBHighRdyPtr;
    MOV32   R1, OSTCBHighRdyPtr
    LDR     R2, [R1]
    STR     R2, [R0]

    LDR     R0, [R2]                                            ; R0为新进程堆栈指针; SP = OSTCBHighRdyPtr->StkPtr;
    MSR     PSP, R0                                             ; 从新进程堆栈指针加载PSP

    MRS     R0, CONTROL
    ORR     R0, R0, #2
    MSR     CONTROL, R0
    ISB                                                         ; 同步指令流

    LDMFD    SP!, {R4-R11, LR}                                  ; 在新进程堆栈中保存 r4-11, lr 
    LDMFD    SP!, {R0-R3}                                       ; 保存寄存器r0, r3
    LDMFD    SP!, {R12, LR}                                     ; 加载R12 和 LR
    LDMFD    SP!, {R1, R2}                                      ; 加载 PC and 禁止 xPSR
    CPSIE    I
    BX       R1


;********************************************************************************************************
;                       执行上下文切换 (从任务级别) - OSCtxSw()
;
; Note(s) : 1) 当操作系统想要执行的任务的上下文切换时，调用OSCtxSw()函数。此函数触发是真正的工作在哪里做的 PendSV 异常.
;********************************************************************************************************

OSCtxSw
    LDR     R0, =NVIC_INT_CTRL                                  ; 触发 PendSV 异常 （引起上下文切换）
    LDR     R1, =NVIC_PENDSVSET
    STR     R1, [R0]
    BX      LR


;********************************************************************************************************
;                   执行上下文切换 (从中断级别) - OSIntCtxSw()
;
; 注意 : 1) OSIntCtxSw()函数由OSIntExit()调用，当它决定上下文切换时需要作为一个中断的结果。
;           此函数只是在没有其他的中断源和中断启用时触发PendSV 异常并被处理。
;********************************************************************************************************

OSIntCtxSw
    LDR     R0, =NVIC_INT_CTRL                                  ; 触发 PendSV 异常 （引起上下文切换）
    LDR     R1, =NVIC_PENDSVSET
    STR     R1, [R0]
    BX      LR


;********************************************************************************************************
;                                       PendSV 异常服务函数
;                                   void OS_CPU_PendSVHandler(void)
;
; 注意    : 1) PendSV用于引起上下文切换。这是Cortex-M执行上下文切换的推荐方法。这是因为Cortex-M在任何异常发生时自动保存处理器上下文的一半, 
;              并在异常返回时恢复原样。所以只需要保存固定的寄存器R4--R11和R14。使用PendSV异常这种方式意味着上下文保存和恢复是相同的
;              无论它从一个线程启动还是发生中断或异常。
;
;           2) 伪代码如下:
;              a) 获取进程堆栈指针；
;              b) 在进程堆栈保存剩余寄存器R4--R11和R14；
;              c) 在任务控制块中保存进程堆栈指针, OSTCBCurPtr->OSTCBStkPtr = SP；
;              d) 调用OSTaskSwHook()函数；
;              e) 获取当前的高优先级, OSPrioCur = OSPrioHighRdy；
;              f) 获取当前就绪的线程 TCB, OSTCBCurPtr = OSTCBHighRdyPtr；
;              g) 从TCB获取新的进程指针SP(PSP), SP = OSTCBHighRdyPtr->OSTCBStkPtr；
;              h) 从新的进程堆栈恢复寄存器R4--R11和R14；
;              i) 执行异常返回，将恢复其余上下文.
;
;           3) 在进入 PendSV 异常的时候:
;              a) 下面8个寄存器由处理器自动压栈:
;                 xPSR, PC, LR, R12, R0-R3
;              b) 处理器模式由用户级变成特权级
;              c) 堆栈由PSP切换成MSP
;              d) OSTCBCurPtr      points to the OS_TCB of the task to suspend
;                 OSTCBHighRdyPtr  points to the OS_TCB of the task to resume
;
;           4) 因为PendSV的优先级最低(由上面的OSStartHighRdy()函数实现),所以PendSV异常服务函数只有在没有其他
;              的异常和中断活跃时才会执行，因此可以有把握地推测被切换出的上下文使用的是进程堆栈 (PSP)。
;********************************************************************************************************

OS_CPU_PendSVHandler
    CPSID   I                                                   ; 关中断，防止中断期间的上下文切换
    MRS     R0, PSP                                             ; 加载PSP(即R13)的值到R0
    STMFD   R0!, {R4-R11, R14}                                  ; 在进程堆栈中保存剩余寄存器r4-11, R14

    MOV32   R5, OSTCBCurPtr                                     ; OSTCBCurPtr->StkPtr = SP;
    LDR     R1, [R5]
    STR     R0, [R1]                                            ; R0 is SP of process being switched out

                                                                ; 在这里，整个上下文的过程已保存
    MOV     R4, LR                                              ; 保存LR exc_return值
    BL      OSTaskSwHook                                        ; 调用OSTaskSwHook()函数为浮点单元入栈和出栈

    MOV32   R0, OSPrioCur                                       ; OSPrioCur   = OSPrioHighRdy;
    MOV32   R1, OSPrioHighRdy
    LDRB    R2, [R1]
    STRB    R2, [R0]

    MOV32   R1, OSTCBHighRdyPtr                                 ; OSTCBCurPtr = OSTCBHighRdyPtr;
    LDR     R2, [R1]
    STR     R2, [R5]

    ORR     LR,  R4, #0x04                                      ; 确保返回的异常使用进程堆栈
    LDR     R0,  [R2]                                           ; R0新进程堆栈指针; SP = OSTCBHighRdyPtr->StkPtr;
    LDMFD   R0!, {R4-R11, R14}                                  ; 从新进程堆栈中恢复寄存器R4-11, R14的值
    MSR     PSP, R0                                             ; 从新进程堆栈中加载PSP
    CPSIE   I
    BX      LR                                                  ; 异常返回时将恢复剩余的上下文

    ALIGN

    END
        