#include <avr/io.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <util/delay.h>
#include <avr/sfr_defs.h>
#include <math.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include "HD44780.h"
#include <float.h>
#include <stdbool.h>


#ifndef _BV
#define _BV(bit)				(1<<(bit))
#endif

#ifndef inb
#define	inb(addr)			(addr)
#endif

#ifndef outb
#define	outb(addr, data)	addr = (data)
#endif

#ifndef sbi
#define sbi(reg,bit)		reg |= (_BV(bit))
#endif

#ifndef cbi
#define cbi(reg,bit)		reg &= ~(_BV(bit))
#endif

#ifndef tbi
#define tbi(reg,bit)		reg ^= (_BV(bit))
#endif

/*
 *  Gotowe zaimplementowane:
 #define 	bit_is_set(sfr, bit)   (_SFR_BYTE(sfr) & _BV(bit))
 #define 	bit_is_clear(sfr, bit)   (!(_SFR_BYTE(sfr) & _BV(bit)))
 #define 	loop_until_bit_is_set(sfr, bit)   do { } while (bit_is_clear(sfr, bit))
 #define 	loop_until_bit_is_clear(sfr, bit)   do { } while (bit_is_set(sfr, bit))

 */

// MIN/MAX/ABS macros
#define MIN(a,b)			((a<b)?(a):(b))
#define MAX(a,b)			((a>b)?(a):(b))
#define ABS(x)				((x>0)?(x):(-x))
//zmienne
char text[20];
int n=0;
volatile uint8_t i,sec,s,min,uno,actual;
volatile bool menu = false;
volatile bool start=true;
char cyfra[10] = { 0b1111110, 0b0110000, 0b1101101, 0b1111001, 0b0110011,
		0b1011011, 0b1011111, 0b1110000, 0b1111111, 0b1111011 };
char close=0b0000000;
//inicjalizacje

void TimerInit() {
	//Wybranie trybu pracy CTC z TOP OCR1A
	sbi(TCCR1B,WGM12);
	//Wybranie dzielnika czestotliwosci
	sbi(TCCR1B,CS12);
	//Zapisanie do OCR1A wartosci odpowiadajacej 0,5s
	OCR1A=625;
	//Uruchomienie przerwania OCIE1A
	sbi(TIMSK,OCIE1A);
}
void seg7Init() {
	//Inicjalizacja kolumn

	//Inicjalizacja segmentu
	DDRC=0xff;
	PORTC=0x00;

}
void ADC_init()
{
	//Rejestr ADMUX bity: REFS0:1 – konfiguracja napiêcia referencyjnego – wybraæ napiêcie AVCC
	sbi(ADMUX,REFS0);
	cbi(ADMUX,REFS1);
	//Rejestr ADCSRA bity: ADPS0:2 – konfiguracja podzielnika czêstotliwoœci dla uk³adu przetwornika (czêstotliwoœæ sygna³u taktuj¹cego) – ustawiæ aby czêstotliwoœæ by³a mniejsza
	//ni¿ 100KHz
	sbi(ADCSRA,ADPS1);
	sbi(ADCSRA,ADPS2);
	cbi(ADCSRA,ADPS0);
	//Rejestr ADCSRA bit: ADEN – uruchomienie uk³adu przetwornika
	sbi(ADCSRA,ADEN);
	//Rejestr ADMUX bity: MUX0:4 – konfiguracja/wybór kana³u/pinu na którym bêdzie dokonywany pomiar – wybraæ ADC0 – odpowiada pinowi PA0
	cbi(ADMUX,MUX0);
	cbi(ADMUX,MUX1);
	cbi(ADMUX,MUX2);
	cbi(ADMUX,MUX3);
	cbi(ADMUX,MUX4);
}
void ButtInit()
{
	cbi(DDRD,PD0);
	cbi(DDRD,PD1);
	cbi(DDRD,PD2);
	cbi(DDRD,PD3);
	sbi(PORTD,PD0);
	sbi(PORTD,PD1);
	sbi(PORTD,PD2);
	sbi(PORTD,PD3);
}
//wyzerowanie wszystkiego
void ground()
{
	uno=0;
	s=0;
	min=0;
	sec=0;
	i=0;
	cbi(PORTA,PA5);
	cbi(PORTA,PA6);
	cbi(PORTA,PA7);

	PORTC=close;
	cli();

}
//przywitanie
void info()
{
	LCD_GoTo(0, 0);
		sprintf(text, "Witaj œwiecie");
		LCD_WriteText(text);
		_delay_ms(4000);
		LCD_Clear();
		menu=false;
}
//licznik
bool is_first(int n)
{
  if(n<2)
    return false; //gdy liczba jest mniejsza ni¿ 2 to nie jest pierwsz¹
 int i;
  for(i=2;i*i<=n;i++)
    if(n%i==0)
      return false; //gdy znajdziemy dzielnik, to dana liczba nie jest pierwsza
  return true;
}
int type()
{
	if(actual%2==0 && actual !=2 && actual !=0)
		return 0;
	else if(actual%2==0 && (actual ==2 || actual ==0))
		return 3;
	if(is_first(actual)==0)
		return 1;
	if((is_first(actual)==1)||actual==1)
		return 2;

}
void counter()
{
	if(bit_is_clear(PIND,PD0)&& actual<50)
	{
		cbi(PORTA,PA5);
		cbi(PORTA,PA6);
		cbi(PORTA,PA7);
		actual++;
		_delay_ms(200);
	}

	if(bit_is_clear(PIND,PD2)&& actual>0)
		{
		cbi(PORTA,PA5);
				cbi(PORTA,PA6);
				cbi(PORTA,PA7);
			actual--;
			_delay_ms(200);
		}
	LCD_GoTo(0, 0);
			sprintf(text, "%d",actual);
			LCD_WriteText(text);
			int ty=type();
			switch (ty)
			{
			case 0:
				sbi(PORTA,PA5);
				break;
			case 1:
				sbi(PORTA,PA6);
				break;
			case 2:
				sbi(PORTA,PA6);
				sbi(PORTA,PA7);
				break;
			case 3:
				sbi(PORTA,PA5);
				sbi(PORTA,PA7);
				break;
			}

}
//funkcje do miernika

uint16_t ADC_10bit() // @suppress("No return")
{
	sbi(ADCSRA,ADSC);
	while (bit_is_set(ADCSRA,ADSC))
	{


	}
	return ADC;
}
void ADC_measure()
{
	uint16_t voltage= ADC_10bit();

	int V=voltage*0.488;
	int calkowita=V/100;
	int ulamkowa= V-calkowita*100;
	sbi(DDRC,PC3);

	LCD_GoTo(0, 0);
	sprintf(text, "Pomiar1: %d.%dV",calkowita,ulamkowa);
	LCD_WriteText(text);

}
//zwraca numer funcji w menu
int choose_opt()
{
	if(menu==false)
	{
	if(bit_is_clear(PIND,PD0))
	{
		LCD_Clear();
		if(n==4)
			n=0;
		else
		n++;
		_delay_ms(200);
	}

	if(bit_is_clear(PIND,PD2))
		{
		LCD_Clear();
			if(n==0)
				n=4;
			else
			n--;
			_delay_ms(200);
		}
	}

	return n;
}
//zatrzymywanie i restartowanie stopera
void srt()
{
	if(bit_is_clear(PIND,PD3)&&start== true)
	{
		start=false;

		_delay_ms(200);
	}
	if(bit_is_clear(PIND,PD3)&&start== false)
	{
		i=0;
				s=0;
				sec=0;
		start=true;
		_delay_ms(200);
	}
}
// wyswietlanie menu
void display()
{

int opt=choose_opt();
switch(opt)
{
case 0:
	LCD_GoTo(0,0);
	sprintf(text, "1.Info");
	LCD_WriteText(text);
	break;
case 1:
	LCD_GoTo(0,0);
	sprintf(text, "2.Liczby");
	LCD_WriteText(text);
	break;
case 2:
	LCD_GoTo(0,0);
	sprintf(text, "3.Stoper");
	LCD_WriteText(text);
	break;
case 3:
	LCD_GoTo(0,0);
	sprintf(text, "4.Zegar");
	LCD_WriteText(text);
	break;
case 4:
	LCD_GoTo(0,0);
	sprintf(text, "5.Miernik");
	LCD_WriteText(text);
	break;

}
//wybor funkcji i jej dzialanie
}
void click()
{

	int opt=choose_opt();
	switch(opt)
	{
	case 0:
		info();
		break;
	case 1:
		sbi(DDRA,PA5);
			sbi(DDRA,PA6);
			sbi(DDRA,PA7);
		counter();

		break;
	case 2:
		sbi(DDRA,PA5);
		cbi(PORTA,PA7);
		LCD_GoTo(0,0);
				sprintf(text, "%d:%d",s,sec);
				sei();
				LCD_WriteText(text);
				srt();
		break;
	case 3:
		sbi(DDRA,PA7);
		cbi(PORTA,PA5);
		start=true;
		sei();
		seg7ShowCyfra(uno);
		LCD_GoTo(0,0);
		sprintf(text, "%d:%d",min,s);
		LCD_WriteText(text);
		break;
	case 4:
		ADC_measure();
		break;

	}
}
//do wyswietlania w wyswietlaczu 7-segmentowym
void seg7ShowCyfra(uint8_t cyfraDoWyswietlenia) {
	PORTC = cyfra[cyfraDoWyswietlenia];	//co to robi - wytlumaczyc prowadzacemu

}

int main() {

	LCD_Initalize();
	ButtInit();

	TimerInit();
	seg7Init();
	i=0;
		LCD_Home();
		ADC_init();
		LCD_GoTo(0,0);
		sprintf(text, "Program PTM 2021");
		LCD_WriteText(text);

		LCD_GoTo(0,1);
		sprintf(text, "252894");
		LCD_WriteText(text);
		_delay_ms(4000);
		LCD_Clear();

	while(1) {
		if(bit_is_clear(PIND,PD1))
		{	LCD_Clear();
			menu=false;
		}
		if(bit_is_clear(PIND,PD3))
				{
					LCD_Clear();

					menu=true;
				}
		if(menu==false)
		{
		display();

		ground();
		}
		if(menu==true)
		{
			click();
		}

	}

}
//przewania
ISR(TIMER1_COMPA_vect) {
if(start==true)
i++;
sec=i*2;
if(sec==100)
{
	s++;
	uno++;
	i=0;
	sec=0;
	tbi(PORTA,PA5);
	tbi(PORTA,PA7);

}
if(s==60)
{
	min++;
	s=0;
}
if(uno%10==0)
{
	uno=0;
}

}
