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
; �ļ���    : OS_CPU_A.ASM
; �汾��    : V3.06.00
; ����      : JJL
;             BAN
;             JBL
;
; CPU �ں�  : ARMv7M Cortex-M
; ָ��ģʽ  : Thumb-2 ISA
; ��������  : MDK(KEIL)
;
; ����+ע��   : ���� fire
; ��̳        : www.firebbs.cn
; �Ա�        : www.fire-stm32.taobao.com
;
; ע��        : (1) �˶˿�֧�� ARM Cortex-M3��Cortex-M4 �� Cortex-M7 �ܹ�.
;               (2) ����Ӳ�����㵥Ԫ�Ѿ�������
;                  (a) ������: FPv4-SP-D16-M �� FPv5-SP-D16-M
;                  (b) ˫����: FPv5-D16-M
;********************************************************************************************************
;

;********************************************************************************************************
;                                          ��������
;********************************************************************************************************

    ; IMPORT��������������������ⲿ�ļ����� C �����е� EXTERN �ؼ�������
	; �ⲿ����
    
    IMPORT  OSPrioCur
    IMPORT  OSPrioHighRdy
    IMPORT  OSTCBCurPtr
    IMPORT  OSTCBHighRdyPtr
    IMPORT  OSIntExit
    IMPORT  OSTaskSwHook
    IMPORT  OS_CPU_ExceptStkBase

    ; EXPORT������������һ����ž���ȫ�����ԣ��ɱ��ⲿ���ļ�ʹ��
	; ���ļ���������
    EXPORT  OSStartHighRdy
    EXPORT  OSCtxSw
    EXPORT  OSIntCtxSw
    EXPORT  OS_CPU_PendSVHandler
    
    ; �Ƿ�ʹ��Ӳ������
    IF {FPU} != "SoftVFP"
    EXPORT  OS_CPU_FP_Reg_Push
    EXPORT  OS_CPU_FP_Reg_Pop
    ENDIF


;********************************************************************************************************
;                                               ��������
;********************************************************************************************************

NVIC_INT_CTRL   EQU     0xE000ED04                              ; �жϿ���״̬�Ĵ���
NVIC_SYSPRI14   EQU     0xE000ED22                              ; ϵͳ���ȼ��Ĵ��� (���ȼ�14)
NVIC_PENDSV_PRI EQU           0xFF                              ; PendSV�����ȼ� (������ȼ�)
NVIC_PENDSVSET  EQU     0x10000000                              ; ����PendSV�쳣��ֵ


;********************************************************************************************************
;                                              ��������ָ��
;********************************************************************************************************

    PRESERVE8       ; 8�ֽڶ���
    THUMB           ; THUMB ָ��ģʽ

; ��AREAαָ�� ���һ������Σ�����ΪCODE����һ����������Ϊֻ��
    AREA CODE, CODE, READONLY


;********************************************************************************************************
;                                   ����Ĵ�����ջ
;                             void  OS_CPU_FP_Reg_Push (CPU_STK  *stkPtr)
;
;ע�� : 1) �˺������渡�㵥Ԫ�ļĴ���S16-S31 ��
;
;       2) α��������:
;          a) �ڽ��̶�ջ��ջ���㵥Ԫ�ļĴ���S16-S31��
;          b) ��������������Ķ�ջ OSTCBCurPtr->StkPtrָ��;
;********************************************************************************************************

    IF {FPU} != "SoftVFP"
        
OS_CPU_FP_Reg_Push
    MRS     R1, PSP                                             ; PSP�ǽ��̶�ջָ��
    CBZ     R1, OS_CPU_FP_nosave                                ; ����FPU�Ĵ�����һ�α���

    VSTMDB  R0!, {S16-S31}
    LDR     R1, =OSTCBCurPtr
    LDR     R2, [R1]
    STR     R0, [R2]
OS_CPU_FP_nosave
    BX      LR
    
    ENDIF


;********************************************************************************************************
;                                   ����Ĵ�����ջ
;                             void  OS_CPU_FP_Reg_Pop (CPU_STK  *stkPtr)
;
; Note(s) : 1) �˺����ָ����㵥Ԫ�ļĴ���S16-S31 ��
;
;           2) α��������:
;              a) �ָ��¹��̶�ջ�ĸ��㵥Ԫ�ļĴ���S16-S31;
;              b) �����½��̶�ջ��OSTCBHighRdyPtr-> StkPtrָ��;
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
;                                         ��ʼ������
;                                      void OSStartHighRdy(void)
;
; ע��:     1) �˺�������PendSV�쳣��ʵ���ϣ������������л������µ�һ������ʼ
;
;           2) ������ִ���ڼ䣬PSP������ջָ��.
;              �������쳣ʱ�����Ľ��л���MSP��ֱ���쳣����
;
;           3) OSStartHighRdy() ���룺
;              a) ��PendSV�쳣���ȼ�����Ϊ��ͣ�
;              b) ��ʼ�����̶�ջָ��PSP����0, �������ǵ�һ���������л���
;              c) ��������ջָ�����OS_CPU_ExceptStkBase��
;              d) ��ȡ��ǰ�ĸ����ȼ�, OSPrioCur = OSPrioHighRdy��
;              e) ��ȡ��ǰ�������߳� TCB, OSTCBCurPtr = OSTCBHighRdyPtr��
;              f) ��TCB��ȡ�½��̶�ջָ��SP, SP = OSTCBHighRdyPtr->StkPtr��
;              g) ���µĽ��̶�ջ�ָ�R4--R11��R14�Ĵ�����
;              h) ʹ���ж� �������������жϺ����У���
;********************************************************************************************************

OSStartHighRdy
    CPSID   I                                                   ; ���жϣ���ֹ�ж��ڼ���������л�
    MOV32   R0, NVIC_SYSPRI14                                   ; ���� PendSV �쳣���ȼ�
    MOV32   R1, NVIC_PENDSV_PRI
    STRB    R1, [R0]

    MOVS    R0, #0                                              ; ��ʼ�����̶�ջָ��PSP����0,�ڳ�ʼ�������л�ʱ����
    MSR     PSP, R0

    MOV32   R0, OS_CPU_ExceptStkBase                            ; ��������ջָ�����OS_CPU_ExceptStkBase
    LDR     R1, [R0]
    MSR     MSP, R1

    BL      OSTaskSwHook                                        ; ����OSTaskSwHook()����Ϊ���㵥Ԫ��ջ�ͳ�ջ

    MOV32   R0, OSPrioCur                                       ; OSPrioCur   = OSPrioHighRdy;
    MOV32   R1, OSPrioHighRdy
    LDRB    R2, [R1]
    STRB    R2, [R0]

    MOV32   R0, OSTCBCurPtr                                     ; OSTCBCurPtr = OSTCBHighRdyPtr;
    MOV32   R1, OSTCBHighRdyPtr
    LDR     R2, [R1]
    STR     R2, [R0]

    LDR     R0, [R2]                                            ; R0Ϊ�½��̶�ջָ��; SP = OSTCBHighRdyPtr->StkPtr;
    MSR     PSP, R0                                             ; ���½��̶�ջָ�����PSP

    MRS     R0, CONTROL
    ORR     R0, R0, #2
    MSR     CONTROL, R0
    ISB                                                         ; ͬ��ָ����

    LDMFD    SP!, {R4-R11, LR}                                  ; ���½��̶�ջ�б��� r4-11, lr 
    LDMFD    SP!, {R0-R3}                                       ; ����Ĵ���r0, r3
    LDMFD    SP!, {R12, LR}                                     ; ����R12 �� LR
    LDMFD    SP!, {R1, R2}                                      ; ���� PC and ��ֹ xPSR
    CPSIE    I
    BX       R1


;********************************************************************************************************
;                       ִ���������л� (�����񼶱�) - OSCtxSw()
;
; Note(s) : 1) ������ϵͳ��Ҫִ�е�������������л�ʱ������OSCtxSw()�������˺��������������Ĺ������������� PendSV �쳣.
;********************************************************************************************************

OSCtxSw
    LDR     R0, =NVIC_INT_CTRL                                  ; ���� PendSV �쳣 �������������л���
    LDR     R1, =NVIC_PENDSVSET
    STR     R1, [R0]
    BX      LR


;********************************************************************************************************
;                   ִ���������л� (���жϼ���) - OSIntCtxSw()
;
; ע�� : 1) OSIntCtxSw()������OSIntExit()���ã����������������л�ʱ��Ҫ��Ϊһ���жϵĽ����
;           �˺���ֻ����û���������ж�Դ���ж�����ʱ����PendSV �쳣��������
;********************************************************************************************************

OSIntCtxSw
    LDR     R0, =NVIC_INT_CTRL                                  ; ���� PendSV �쳣 �������������л���
    LDR     R1, =NVIC_PENDSVSET
    STR     R1, [R0]
    BX      LR


;********************************************************************************************************
;                                       PendSV �쳣������
;                                   void OS_CPU_PendSVHandler(void)
;
; ע��    : 1) PendSV���������������л�������Cortex-Mִ���������л����Ƽ�������������ΪCortex-M���κ��쳣����ʱ�Զ����洦���������ĵ�һ��, 
;              �����쳣����ʱ�ָ�ԭ��������ֻ��Ҫ����̶��ļĴ���R4--R11��R14��ʹ��PendSV�쳣���ַ�ʽ��ζ�������ı���ͻָ�����ͬ��
;              ��������һ���߳��������Ƿ����жϻ��쳣��
;
;           2) α��������:
;              a) ��ȡ���̶�ջָ�룻
;              b) �ڽ��̶�ջ����ʣ��Ĵ���R4--R11��R14��
;              c) ��������ƿ��б�����̶�ջָ��, OSTCBCurPtr->OSTCBStkPtr = SP��
;              d) ����OSTaskSwHook()������
;              e) ��ȡ��ǰ�ĸ����ȼ�, OSPrioCur = OSPrioHighRdy��
;              f) ��ȡ��ǰ�������߳� TCB, OSTCBCurPtr = OSTCBHighRdyPtr��
;              g) ��TCB��ȡ�µĽ���ָ��SP(PSP), SP = OSTCBHighRdyPtr->OSTCBStkPtr��
;              h) ���µĽ��̶�ջ�ָ��Ĵ���R4--R11��R14��
;              i) ִ���쳣���أ����ָ�����������.
;
;           3) �ڽ��� PendSV �쳣��ʱ��:
;              a) ����8���Ĵ����ɴ������Զ�ѹջ:
;                 xPSR, PC, LR, R12, R0-R3
;              b) ������ģʽ���û��������Ȩ��
;              c) ��ջ��PSP�л���MSP
;              d) OSTCBCurPtr      points to the OS_TCB of the task to suspend
;                 OSTCBHighRdyPtr  points to the OS_TCB of the task to resume
;
;           4) ��ΪPendSV�����ȼ����(�������OSStartHighRdy()����ʵ��),����PendSV�쳣������ֻ����û������
;              ���쳣���жϻ�Ծʱ�Ż�ִ�У���˿����а��յ��Ʋⱻ�л�����������ʹ�õ��ǽ��̶�ջ (PSP)��
;********************************************************************************************************

OS_CPU_PendSVHandler
    CPSID   I                                                   ; ���жϣ���ֹ�ж��ڼ���������л�
    MRS     R0, PSP                                             ; ����PSP(��R13)��ֵ��R0
    STMFD   R0!, {R4-R11, R14}                                  ; �ڽ��̶�ջ�б���ʣ��Ĵ���r4-11, R14

    MOV32   R5, OSTCBCurPtr                                     ; OSTCBCurPtr->StkPtr = SP;
    LDR     R1, [R5]
    STR     R0, [R1]                                            ; R0 is SP of process being switched out

                                                                ; ��������������ĵĹ����ѱ���
    MOV     R4, LR                                              ; ����LR exc_returnֵ
    BL      OSTaskSwHook                                        ; ����OSTaskSwHook()����Ϊ���㵥Ԫ��ջ�ͳ�ջ

    MOV32   R0, OSPrioCur                                       ; OSPrioCur   = OSPrioHighRdy;
    MOV32   R1, OSPrioHighRdy
    LDRB    R2, [R1]
    STRB    R2, [R0]

    MOV32   R1, OSTCBHighRdyPtr                                 ; OSTCBCurPtr = OSTCBHighRdyPtr;
    LDR     R2, [R1]
    STR     R2, [R5]

    ORR     LR,  R4, #0x04                                      ; ȷ�����ص��쳣ʹ�ý��̶�ջ
    LDR     R0,  [R2]                                           ; R0�½��̶�ջָ��; SP = OSTCBHighRdyPtr->StkPtr;
    LDMFD   R0!, {R4-R11, R14}                                  ; ���½��̶�ջ�лָ��Ĵ���R4-11, R14��ֵ
    MSR     PSP, R0                                             ; ���½��̶�ջ�м���PSP
    CPSIE   I
    BX      LR                                                  ; �쳣����ʱ���ָ�ʣ���������

    ALIGN

    END
        