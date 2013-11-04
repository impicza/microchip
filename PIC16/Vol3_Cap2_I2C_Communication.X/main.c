/* 
 * File:   main.c
 * Author: Roney
 *
 * Created on 29 de Julho de 2013, 13:49
 *
 * Capitulo 2 do Livro Chuck Hellebuyck, Volume 3
 *
 * Cria��o de Bibliotecas I2C para exemplo
 * em Memoria EEPROM, adaptado para a placa
 * ACEPIC28, com Pic26f886 e Memoria AT-24C04
 *
 */

#include <htc.h>
#include <stdio.h>

//#include <stdlib.h>

#define _LEGACY_HEADERS

#define _XTAL_FREQ 8000000             // Used in __delay_ms() functions

__CONFIG(HS & WDTDIS & MCLREN & UNPROTECT & LVPDIS & IESODIS & FCMDIS);


void init_micro(void)
{
    ANSEL = 0;
    ANSELH = 0;
    CM1CON0 = 0;
    CM2CON0 = 0;
    INTCON = 0;
}

void msecbase (void)
{
    OPTION_REG = 0b00000010; // 0010 para 8 mhz e 0011 para 16 mhz
    TMR0 = 0xD;
    while (!T0IF);
    T0IF=0;
}

void pause (unsigned long usvalue)
{
    unsigned long x;
    for (x=1; x <= (usvalue); x++)
    {
        msecbase();
    }
}

// Hardware I2C - 6 funcoes



void i2c_init ()
{
    TRISC3=1;       // configura o SCL e SDA
    TRISC4=1;       //

    SSPCON = 0x38;  // configura o I2C como Master Mode
    SSPCON2= 0x00;

    //SSPADD = 0x6; // para barramento 400 khz e cristal de 10 mhz
                    // para cristal de 20mhz, usar 0xC0

    // I2C Master mode, clock = FOSC/(4 * (SSPADD + 1))
    //SSPADD = 39;   (27 hex) para 100Khz @ 16Mhz Fosc

    SSPADD = 0x27; // para 100 khz e clock de 8 mhz

    SMP = 1;        // disabilita o SLEW RATE CONTROL para 100 khz

    SSPIF=0;        // limpa o flag de interrupcao SSPIF
    BCLIF=0;        // limpa o flag de COLISAO do barramento
}

// SSPSTAT.2 = bit 2 = 100 = 4
void i2c_waitForIdle()
{
    while ( ( SSPCON2 & 0x1F) | SSPSTAT & 0b100 ) {};    // espera idle e not-writing
}

void i2c_start()
{
    i2c_waitForIdle();
    SEN=1;
}

void i2c_repStart()
{
    i2c_waitForIdle();
    RSEN=1;
}

void i2c_stop()
{
    i2c_waitForIdle();
    PEN=1;
}

int i2c_read (unsigned char ack )
{
    unsigned char i2cReadData;

    i2c_waitForIdle();
    RCEN=1;
    i2c_waitForIdle();
    i2cReadData = SSPBUF;
    i2c_waitForIdle();

    if (ack)
    {
        ACKDT=0;
    }
    else
    {
        ACKDT=1;
    }
    ACKDT=1;    // envia a sequencia do ACK

    return (i2cReadData);
}

unsigned char i2c_write (unsigned char i2cWriteData)
{
    i2c_waitForIdle();
    SSPBUF = i2cWriteData;

    return ( ! ACKSTAT);    // funcao retorna 1 se a transmissao for confirmada
}

// escreve endereco de 8 bits na EEPROM com endereco de 8 bytes (2k bit max)
void I2C_EEOUT (unsigned char address, unsigned char data)
{
    i2c_start();
    i2c_write(0xA0);
    i2c_write(address);
    i2c_write(data);
    i2c_stop();
    pause(11);

}

// le do endereco de 8 bits, com endereco de 8 bytes (2k bit max)
unsigned char I2C_EEIN (unsigned char address)
{
    unsigned char data;

    i2c_start();
    i2c_write(0xA0);
    i2c_write(address);
    i2c_repStart();
    i2c_write(0xA1);
    data=i2c_read(0);
    i2c_stop();
    return(data);

}



/*
 * 
 */
void main(void) {

    init_micro();
    i2c_init();

    PORTB=0x00; // todos LEDs desligados
    TRISB=0x00; // todos pinos como outputs
    
    unsigned char cnt=0;
    while (cnt <= 20)   // escreve 10 numeros na EEPROM
    {
        PORTB=cnt;
        I2C_EEOUT(cnt,10-cnt); // coloca 9 no endereco 1, 8 no 2, etc
        cnt=cnt+1;
        pause(255);
    }
    
    cnt = 0;
    
    while (cnt<=10)
    {
        PORTB = I2C_EEIN(cnt);
        pause (1000);
        cnt=cnt+1;
    }

    //return (0);
}

