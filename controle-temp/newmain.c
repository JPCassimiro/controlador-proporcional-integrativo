#include <xc.h>
#include "nxlcd.h"
#include "nxlcd.c"
#pragma config FOSC = HS // Fosc = 20MHz; Tcy = 200ns
#pragma config CPUDIV = OSC1_PLL2 // OSC/1 com PLL off
#pragma config WDT = OFF // Watchdog desativado
#pragma config LVP = OFF // Desabilita gravação em baixa
#pragma config DEBUG = ON // Habilita debug
#pragma config MCLRE = ON // Habilita MCLR
#define _XTAL_FREQ 20000000

int flagAdFim = 0;
int contRa3 = 0;
float convertRes = 0;
float convertPot = 0;
int display = 0;
float erro = 0;
float erro_ant = 0;
float sai_cont = 0;
float sai_cont_ant = 0;
float A = 2;
float B = -1.999333333333333;
int dc = 0;
float tBase = 35;
float convertResTemp = 0;
float convertDc = 0;
void interrupt HighPriorityISR(void) {
    PIR2bits.TMR3IF = 0;
    if (contRa3 < 49) {//ESCOLHENDO A PORTA RA3(le o potenciometro)
        ADCON0bits.CHS3 = 0; //ESCOLHENDO Porta
        ADCON0bits.CHS2 = 0;
        ADCON0bits.CHS1 = 0;
        ADCON0bits.CHS0 = 0;
        ADCON1bits.VCFG0 = 0; //VALOR DE REF

    } else {//ESCOLHENDO A PORTA RA0(le o resistor)
        ADCON0bits.CHS3 = 0; //ESCOLHENDO Porta
        ADCON0bits.CHS2 = 0; //1500/30 = 50 PODEMOS USAR O CONTADOR DE 1500 PARA FAZER UM CONTADOR DE 30HZ
        ADCON0bits.CHS1 = 1;
        ADCON0bits.CHS0 = 1;
        ADCON1bits.VCFG0 = 0;
    }
    TMR3H = 0xF2; //62203
    TMR3L = 0xFB;
    ADCON0bits.GODONE = 1;
    contRa3++;
}

void interrupt low_priority LowPriorityISR(void) {
    PIR1bits.ADIF = 0;
    flagAdFim = 1;
}

void main(void) {
    //PWM RC2 GERAÇÃO DE SINAL PARA VENTOINHA/////////////////////////
    TRISC = 0;

    //config ccp2 como pwm
    CCP1CONbits.CCP1M3 = 1;
    CCP1CONbits.CCP1M2 = 1;
    CCP1CONbits.CCP1M1 = 0;
    CCP1CONbits.CCP1M0 = 0;

    //timer2 config
    T2CONbits.T2OUTPS3 = 0; //postscale - 0
    T2CONbits.T2OUTPS2 = 0;
    T2CONbits.T2OUTPS1 = 0;
    T2CONbits.T2OUTPS0 = 0;
    T2CONbits.TMR2ON = 1; //enable
    T2CONbits.T2CKPS1 = 0; //ps - 4
    T2CONbits.T2CKPS0 = 1;
    PR2 = 199; // (Tpwm/Tosc*PS) - 1

    /*dc = ((PR2+1)*4);
    CCPR1L = (char) (dc >> 2); //atribui 8 bits de razão
    CCP1CONbits.DC1B0 = dc % 2; //atribui 2
    CCP1CONbits.DC1B1 = (dc >> 1) % 2;*/
    /////////////////////////////////////////////////////////////////

    //CONVERSOR AD RA0//////////////////////////////////////////////
    //INTERRUPT
    RCONbits.IPEN = 1;
    INTCONbits.GIEH = 1;
    INTCONbits.GIEL = 1;
    ///////////

    TRISA = 0xFF;
    //timer3
    T3CONbits.RD16 = 1; //TIMER3 EM 16 BITS PQ 1500HZ NÃO CABE EM 8
    T3CONbits.T3CKPS1 = 0; //PRESCALER DE 1 JÁ É SUFICIENTE
    T3CONbits.T3CKPS0 = 0;
    T3CONbits.TMR3CS = 0; //ESCOLHE CLOCK INTERNO
    T3CONbits.TMR3ON = 1; //LIGA
    TMR3H = 0xF2; //62203
    TMR3L = 0xFB;
    PIR2bits.TMR3IF = 0; //FLAG
    PIE2bits.TMR3IE = 1; //ENABLE INTERRUPT
    IPR2bits.TMR3IP = 1; //PRIO ALTA 

    /////
    //CONVERSOR AD
    /*ADCON0bits.CHS3 = 0; //ESCOLHENDO PINO
    ADCON0bits.CHS2 = 0;
    ADCON0bits.CHS1 = 0;
    ADCON0bits.CHS0 = 0;*/
    ADCON0bits.ADON = 1; //LIGA
    ADCON0bits.GODONE = 0; //"FLAG"
    ADCON1bits.VCFG1 = 0; //VALORES DE REFERNECIA BAIXA DEFAULT
    ADCON1bits.VCFG0 = 0; //VALORES DE REFERNECIA ALTA VEM DA PORTA RA3 1
    ADCON1bits.PCFG3 = 1; //configitação das portas
    ADCON1bits.PCFG2 = 0; //PORTAS 0 A 3 SÃO ANALOGICAS, OUTRAS SÃO DIGITAS
    ADCON1bits.PCFG1 = 1; //
    ADCON1bits.PCFG0 = 1; //
    PIR1bits.ADIF = 0; //FLAG INTERRUPT
    PIE1bits.ADIE = 1; //ENABLE INTERRUPT
    IPR1bits.ADIP = 0; //PRIO BAIXA
    ADCON2bits.ADFM = 1; //justificativa
    ADCON2bits.ACQT2 = 0; //TAXA DE Acquisition
    ADCON2bits.ACQT1 = 1; //
    ADCON2bits.ACQT0 = 0; //
    ADCON2bits.ADCS2 = 1; //SELEÇÃO DE CLOCK
    ADCON2bits.ADCS1 = 0; //
    ADCON2bits.ADCS0 = 1; //
    ////
    /////////////////////////////////////////////////////////////////
    //PORT C RESITOR/////////////////////////////////////////////////
    PORTCbits.RC1 = 1; //RESISTOR DEVE PERMANER EM 100% DE POTENCIA
    /////////////////////////////////////////////////////////////////



    //DISPLAY LCD////////////////////////////////////////////////////
    TRISD = 0;
    TRISE = 0;
    OpenXLCD(FOUR_BIT & LINES_5X7);
    WriteCmdXLCD(0x01);
    __delay_ms(2);
    WriteCmdXLCD(0x01);
    WriteCmdXLCD(0x0C);
    /////////////////////////////////////////////////////////////////
    while (1) {
        if ((flagAdFim == 1) && (contRa3 < 49)) {//terminar parte 4, controle do PI - DUTY CICLE
            flagAdFim = 0; //CONVERSÃO AD DO RESITOR TERMINOU

            convertRes = (4.887585533 * ((256 * ADRESH) + ADRESL)); //5000/1024 - OBTEM O VALOR DA TEMPERATURA CONVERTIDA
            convertResTemp = (float)convertRes*0.1;

            erro = -(convertPot - convertResTemp);//exemplo 1.3
            sai_cont = (erro * A) + (erro_ant * B) + (sai_cont_ant);

            if (sai_cont > 800) { 
                sai_cont = 800;
            }
            if (sai_cont < 0) {
                sai_cont = 0;
            }
            sai_cont_ant = sai_cont;
            erro_ant = erro;       
          
            dc = (int)sai_cont;
            CCPR1L = (char) (dc >> 2); //atribui 8 bits de razão
            CCP1CONbits.DC1B0 = dc % 2; //atribui 2
            CCP1CONbits.DC1B1 = (dc >> 1) % 2;
        }
        if ((flagAdFim == 1) && (contRa3 > 49)) {//COVNERSÃO AD DO POTENCIOMENTRO TERMINOU
            flagAdFim = 0;
            contRa3 = 0;

            convertPot = 0.01466275659 * ((256 * ADRESH) + ADRESL);
            convertPot = convertPot + tBase;
            
            display = convertPot*10;//425
            
            WriteCmdXLCD(0x80);
            putrsXLCD("T.REF:");
            putcXLCD(0x30 + (display / 100));//4
            display = display % 100;//250
            putcXLCD(0x30 + (display / 10));//2
            putrsXLCD(".");
            putcXLCD(0x30 + (display % 10));//5 //USA 10 ESPAÇOS
            

            putrsXLCD("  DC:");

            display = convertResTemp*10;
            
            WriteCmdXLCD(0xC0);
            putrsXLCD("T.LIDA:");
            putcXLCD(0x30 + (display / 100));
            display = display % 100;
            putcXLCD(0x30 + (display / 10));
            putrsXLCD(".");
            putcXLCD(0x30 + (display % 10)); //USA 10 ESPAÇOS

            putrsXLCD(" ");
            convertDc = 0.125*dc;
            display = convertDc;
            putcXLCD(0x30 + display / 100);
            display = display % 100;
            putcXLCD(0x30 + display / 10);
            putcXLCD(0x30 + display % 10);
            putrsXLCD("%");
        }
    }
}


//TODA VEZ QUE INTERRUPT BAIXA ESTOURAR TROCAR DUTY CICLY PWM
//LOGICA DO DISPLAY(maior parte feita)

//ATUALMENTE FUNCIONADO
//RESISTOR EM 100%
//PWM VENTOINHA
//CONTADOR DE 30HZ NO INTERRUPT ALTO
//TROCAR A PORTA DO AD TODA VEZ QUE TEM QUE FAZER UMA LEITURA
//LEITURA


//perguntar
//como configurar a tempereratura do sinal de referencia
//se vai ter que fazer o caminho contrario pra voltagem pra temperatura do potenciometro
//como arruma o projeto no proteus
//como ajusta o dc