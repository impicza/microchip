/*
 * File:   main.c
 * Author: roney
 *
 * Created on 21 de Mar�o de 2014, 16:53    -   Versao 0.1
 * Versao 0.2  - 23mar2014 - implementada comunicacao RS232 e I2C
 *               ainda desajustado com frequencia, SDA e SCL.
 * Versao 0.2a - 24mar2014 - I2C  nao funcionando, SLAVE nao respondendo.
 *               NAKs. troca endereco de 8B para 5C
 * Versao 0.2b - removidas rotinas experimentais desnecessarias.
 * Versao 0.2c - 25mar2014 - I2C/SCL nao funcionam de jeito nenhum.
 *               adicionado RTC com circuito DS1307, e tambem nao funciona.
 *               obs: usando um Arduino, tudo funciona normalmente, sem dor.
 *
 */

#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config_bits.h"
#include <plib/i2c.h>
#include <plib/usart.h>
//#include <plib/rtcc.h>

#define _XTAL_FREQ 8000000
#define FOSC 8000000UL
#define Baud 100000

#define LED_VERM PORTDbits.RD7  // Led recebera pisca-pisca via software while
#define LED_AMAR PORTDbits.RD6  // Led recebera pisca-pisca via software while
#define LED_VERD1 PORTDbits.RD3  // Led recebera PISCA 1 Segundo via TIMER1 16bit
#define LED_VERD2 PORTDbits.RD2  // Led recebera PISCA 0,1 Segungo via TIMER0 8bit
#define LED_AZUL PORTCbits.RC2  //

// 0x5C =  1011100  ; 5C <<1
// 0xB8 =  10111000

/*
        AckI2C      :   Generate I2C? bus Acknowledgecondition.
        CloseI2C    :   Disable the SSP module.
        DataRdyI2C  :   Is the data available in the I2C buffer?
        getcI2C     :   Read a single byte from the I2C bus.
        getsI2C     :   Read a string from the I2C bus operating in master I2C mode.
        IdleI2C     :   Loop until I2C bus is idle.
        NotAckI2C   :   Generate I2C bus Not Acknowledgecondition.
        OpenI2C     :   Configure the SSP module.
        putcI2C     :   Write a single byte to the I2C bus.
        putsI2C     :   Write a string to the I2C bus operating in either Master or Slave mode.
        ReadI2C     :   Read a single byte from the I2C bus.
        RestartI2C  :   Generate an I2C bus Restartcondition.
        StartI2C    :   Generate an I2C bus Startcondition.
        StopI2C     :   Generate an I2C bus Stopcondition.
        WriteI2C    :   Write a single byte to the I2C bus.
 */

void ciclo (void);
void readDevice2(void);


/*
 *
 */

//SSPCON1 = 0x28;	// I2C ENB, FOSC / (4 * (SSPADD+1))

int main(int argc, char** argv) {

    //OSCCONbits.IRCF = 0b111; // clock interno para 8 mhz
    //Oscilador Interno nao tem precisao para gerar Clock I2C
    // Nao deve ser utilizado

    unsigned char data;
    signed char status;

    int hora, minuto, segundo, diasemana, dia, mes, ano, dummy;
    int cont=10;
    char msg[40];


    // Definicao de portas para os LEDs
    TRISDbits.RD7=0; TRISDbits.RD6=0; TRISDbits.RD3=0; TRISDbits.RD2=0;
    TRISCbits.RC2=0;

    TRISBbits.RB0=1;    //SDA
    TRISBbits.RB1=1;    //SCL

    TRISCbits.RC6=0;    // Saida TX EUSART

    ADCON1=0x0F;

    PORTD=0x00; PORTCbits.RC2=0;

    SSPCON1bits.SSPEN=1;
    SSPCON2=0x00;

    CMCON=0x00;



CKE=0;  // use I2C levels worked also with '0'
SMP=1;  // disable slew rate control worked also with '0'
PSPIF=0;  // clear SSPIF interrupt flag
BCLIF=0;  // clear bus collision flag


    SSPCON1        = 0x36;          // SSPEN: Synchronous Serial Port Enable bit - Enables the serial port and configures the SDA and SCL pins as the serial port pins
                                    // CKP: SCK Release Control bit              - Release clock
                                    // SSPM3:SSPM0: SSP Mode Select bits         - 0110 = I2C Slave mode, 7-bit address
    SSPSTAT        = 0x00;
    //SSPCON2        = 0x01;          // GCEN: General Call address (00h) (Slave mode only) 0 = General call address disabled
                                    // SEN: Start Condition Enable/Stretch Enable bit(1) ()Slave mode) 1 = Clock stretching is enabled
    PIR1bits.SSPIF = 0;             // Clear MSSP interrupt request flag
    PIE1bits.SSPIE = 1;             // Enable MSSP interrupt enable bi
    INTCONbits.GIE_GIEH  = 1;       // GIE/GIEH: Global Interrupt Enable bit
    INTCONbits.PEIE_GIEL = 1;       // PEIE/GIEL: Peripheral Interrupt Enable bi


    //SSPADD = 0x27; // para 100 khz e clock de 8 mhz

    SMP = 1;        // disabilita o SLEW RATE CONTROL para 100 khz

    SSPIF=0;        // limpa o flag de interrupcao SSPIF
    BCLIF=0;        // limpa o flag de COLISAO do barrament


    // SSPADD = (FOSC/ Bit Rate) /4 - 1
    // SSPADD = ( 4.000.000/100.000 )/4 -1
    // SSPADD = ( 40 ) / 4 -1 = 10-1 = 9

    //SSPADD = ((FOSC/4)/Baud)-1;
    //SSPADD =  ( 4000000/4 / 100000) -1;
    //SSPADD =    1.000.000 / 100.000  - 1
    //SSPADD = 10 - 1 = 9

    // (8.000.000 / 4) / 100.000   - 1;
    //  2.000.000
    //  --------- = 20 - 1 = 19
    //    100.000

    SSPADD = 19;



    CloseUSART();   // fecha qualquer USART que estaria supostamente aberta antes
                    // just closes any previous USART open port

    OpenUSART(  USART_TX_INT_OFF &
                USART_RX_INT_OFF &
                USART_ASYNCH_MODE &
                USART_EIGHT_BIT &
                USART_CONT_RX &
                USART_BRGH_LOW,
                12
                );
                // 51 = 1200bps@4mhz;  6=9600bps@4mhz (Err);  25=2400bps@4mhz
                // 103=1200bps@8mhz; 25=4800bps@8mhz; 51=2400bps@8mhz; 12=9600bps@8mhz

                while(BusyUSART());
                putrsUSART("\n\rINIT SERIAL; SSPAD=");

                while(BusyUSART());
                putsUSART( itoa(NULL,SSPADD,10) );

                while(BusyUSART());
                putrsUSART(" (hex=0x");
                putrsUSART( itoa(NULL,SSPADD,16) );
                putrsUSART(")\n\r");

    LED_VERM = 1;

    Delay10KTCYx(100);Delay10KTCYx(100);

    OpenI2C(MASTER,SLEW_OFF);
    SSPADD=19;


    

    while ( cont>=0 )

    {


    IdleI2C();
    StartI2C();
    IdleI2C();
        WriteI2C( 0xD0 );
        IdleI2C();
        WriteI2C( 0x00 );

        IdleI2C();
        //AckI2C();
    StopI2C();

    IdleI2C();
    RestartI2C();
    IdleI2C();
        WriteI2C( 0xD1 );
        IdleI2C();

        while(!DataRdyI2C());

        hora=ReadI2C();
        minuto=ReadI2C();
        segundo=ReadI2C();
        diasemana=ReadI2C();
        dia=ReadI2C();
        mes=ReadI2C();
        ano=ReadI2C();
        dummy=ReadI2C();
        NotAckI2C();
        IdleI2C();
    StopI2C();

    Delay10KTCYx(100);Delay10KTCYx(100);
    sprintf(msg,"%i,%i,%i,%i,%i,%i,%i,%i\r\n", hora,minuto,segundo,diasemana,
            dia,mes,ano,dummy);

    while(BusyUSART());
                putsUSART( msg );





    cont--;
    Delay10KTCYx(100);Delay10KTCYx(100);
    }

    LED_AMAR = 1;
    Delay10KTCYx(50);
    //#define StartI2C()  SSPCON2bits.SEN=1;while(SSPCON2bits.SEN)

    /*
    	do
	{
	status = WriteI2C( 0xB8 | 0x00 );	//write the address of slave
		if(status == -1)		//check if bus collision happened
		{
                    while(BusyUSART());
                        putrsUSART("\n\rColisao.\n\r");

			data = SSPBUF;		//upon bus collision detection clear the buffer,
			SSPCON1bits.WCOL=0;	// clear the bus collision status bit
		}
	}
	while(status!=0);		//write untill successful communication
     */

        LED_VERM=0; LED_AMAR=0; LED_VERD1=1;
        Delay10KTCYx(50);
        LED_VERD1=0;

    // ---//---

    //while ( SSPCON2bits.SEN );


    // ---//---

    /*
    if (DataRdyI2C())
    {
        unsigned char data = getcI2C();
        LED_VERD1=1;
        Delay10KTCYx(100);
        LED_VERD1=0;
        while(BusyUSART());
            putcUSART(data);
    }*/

        // reading humidity
        //rx_byte =  i2c_read(0x00);
        //rx_byte1 = i2c_read(0x01);

        //read temp
        //rx_byte2 = i2c_read(0x02);
        //rx_byte3 = i2c_read(0x03);

        //printf ("rx_byte1 = 0x%2.2X\r   0x%2.2X\r \n", rx_byte, rx_byte1);
        //printf ("rx_byte1 = 0x%2.2X\r   0x%2.2X\r \n", rx_byte2, rx_byte3);


    //IdleI2C();

    //getcI2C();

    //WriteI2C();

    //LED_AMAR=1;
    //CloseI2C();

    //Delay10KTCYx(100);
    //LED_AMAR=0; LED_VERD1=1;
    //Delay10KTCYx(100); LED_VERD1=0;

    

    GIE=0;  // Desabilitando Interrupcoes Globais

    ciclo();

    while (1);


}

void readDevice2(void){

    /*
     I2C1_Start();              // issue I2C start signal
     I2C1_Write(0x5C);          // send byte via I2C  (device address + W)

     I2C1_Write(0x03);          // send byte (address of EEPROM location)
     I2C1_Write(0x00);          // send data (data to be written)
     I2C1_Write(0x04);
     I2C1_Stop();               // issue I2C stop signal

     I2C1_Start();              // issue I2C start signal
     I2C1_Write(0x5C);          // send byte via I2C  (device address + W)
     I2C1_Write(0x03);          // send byte (address of EEPROM location)
     I2C1_Write(0x00);          // send data (data to be written)
     I2C1_Write(0x04);
     I2C1_Stop();

     I2C1_Start();                  // issue I2C start signal
     I2C1_Write(0x5C);              // send byte via I2C  (device address + W)
     dummy = I2C1_Read(1);          // Read the data (acknowledge)
     dummy = I2C1_Read(1);          // Read the data (acknowledge)
     FC = I2C1_Read(1);             // Read the data (acknowledge)
     RN = I2C1_Read(1);             // Read the data (acknowledge)
     RHL = I2C1_Read(1);            // Read the data (acknowledge)
     RHH = I2C1_Read(1);            // Read the data (acknowledge)
     TEMPL = I2C1_Read(1);          // Read the data (acknowledge)
     TEMPH = I2C1_Read(1);          // Read the data (acknowledge)
     I2C1_Stop();

     RH = RHH << 8;
     RH |= RHL;

     TEMP = TEMPH << 8;
     TEMP |= TEMPL;
     */

}



void ciclo (void)
{
    unsigned char TEMPL=0, TEMPH=0, PRESSAOL=0, PRESSAOH=0;
    unsigned char DUMMY=0, OP=0, BT=0;
    char msg[55];

    Delay10KTCYx(100); //Passing 0 (zero) results in a delay of 2,560,000 cycles

    while(BusyUSART());
    putrsUSART("\n\rINIT SERIAL.\n\r");

    while (1)
    {
        //putrsUSART("\r\n____OPEN I2C; ");
        //OpenI2C(MASTER,SLEW_OFF);
        LED_VERM=1;

        while(BusyUSART());
        putrsUSART("\r\n____START I2C.\n\r");

        StartI2C();             // ACORDAR DEVICE
            WriteI2C(0xB8);     // endereco Slave do AM2315
            //WriteI2C(0x03);     // byte que simboliza a temperatura
            //WriteI2C(0x00);     // start byte para leitura
            //WriteI2C(0x00);     // quantidades de bytes a serem lidos;
            //Delay1KTCYx(2);    // 2 milisegundos (800 us a 3 ms)
            __delay_us(135);
        StopI2C();

        // 10K (100) = 1000 ms
        // 1K  (100) = 100 ms
        // 1K  (10)  = 10 ms
        // 1K  (2)   = 2 ms
        // Delay100TCYx();
        __delay_us(25);


        RestartI2C();             // REQUISITAR PEDIDO DE BYTES
            WriteI2C(0xB8);     // endereco Slave do AM2315
            WriteI2C(0x03);     // byte que simboliza a temperatura
            WriteI2C(0x00);     // start byte para leitura
            WriteI2C(0x04);     // quantidades de bytes a serem lidos;
            //AckI2C();
        StopI2C();

        //AckI2C();
        __delay_ms(10);
        //LED_VERD2=1;
        // Delay10KTCYx(1);

        RestartI2C();
        WriteI2C(0xB9);     // endereco Slave do AM2315
        //__delay_us(39);

        OP          = ReadI2C();        // 1 byte
        BT          = ReadI2C();        // 2 byte
            TEMPL       = ReadI2C();    // 3 byte
            TEMPH       = ReadI2C();    // 4 byte
            PRESSAOL    = ReadI2C();    // 5 byte
            PRESSAOH    = ReadI2C();    // 6 byte
        DUMMY          = ReadI2C();     // 7 byte
        DUMMY          = ReadI2C();     // 8 byte
        StopI2C();

        //AckI2C();


        LED_VERM=0; LED_AMAR=0;LED_VERD1=0;LED_VERD2=0;
        LED_AZUL=1;

        sprintf (msg, "OP=%2.2X BT=%2.2X PL=%2.2X %2.2X TL=%2.2X %2.4X Dy=%2.4X \r\n",OP,BT,PRESSAOH,PRESSAOL,TEMPL,TEMPH,DUMMY);
        //printf ("TEMPL = 0x%2.2X TEMPH= 0x%2.2X \r ",TEMPL,TEMPH);

        while(BusyUSART());
        //putrsUSART(msg);
        putsUSART(msg);

        //,

        Delay10KTCYx(100); LED_AZUL=0;
        while(BusyUSART());
        putrsUSART("\n\r");
        Delay10KTCYx(100);Delay10KTCYx(100);Delay10KTCYx(100);Delay10KTCYx(100);
        Delay10KTCYx(100);Delay10KTCYx(100);Delay10KTCYx(100);Delay10KTCYx(100);

        /*Delay10KTCYx(100);Delay10KTCYx(100);Delay10KTCYx(100);
        Delay10KTCYx(100);Delay10KTCYx(100);*/

    }



}