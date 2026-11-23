/*********************************************/
/*                                           */
/* Programa de juego de Lander By Oscar Flor */
/*                                           */
/*********************************************/
/* I4A - 1996                                */
/* Alumno: Oscar Flor - 115061               */
/*********************************************/

/* Librer¡as a llamar */
#include <graphics.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <dos.h>

/* Constantes */
const Verdadero = 1;
const Falso = 0;

/* Variables Globales */
void *Lander;
void *Meteoro;
void *Explo1;
void *Explo2;
void *LanderAnt;
void *MeteoroAnt;
int xLander;
int yLander;
int xLanderAnt;
int yLanderAnt;
int xMeteoro;
int yMeteoro;
int xMeteoroAnt;
int yMeteoroAnt;

/* Rutina principal */
void main()
{
	/* Declaraci¢n de funciones a llamar */
	int IniciaPrograma(void);
	void Proceso(void);
	void FinPrograma(void);
	void VerError(char Mensaje[80]);

	/* Inicio del programa */
	if (IniciaPrograma() == Falso)
		VerError("Fallo de iniciaci¢n del programa");
	else
		{
			Proceso();
			FinPrograma();
		}
}

/* Funci¢n para iniciar el programa */
int IniciaPrograma(void)
{
	/* Declaraci¢n de funciones a llamar */
	int IniciaGrafico(void);
	void VerError(char Mensaje[80]);
	void DibujaPantalla(void);
	void DefinirPersonajes(void);
	void IniciarJuego(void);

	if (IniciaGrafico() == Falso)
		{
			VerError("Fallo al iniciar modo gr fico");
			return(Falso);
		}
	else
		DefinirPersonajes();
		DibujaPantalla();
		IniciarJuego();
		return(Verdadero);
}

/* Funci¢n para definir a los personajes */
void DefinirPersonajes(void)
{
	/* Determina el area a guardar  */
	unsigned int Tamano = imagesize(100, 100, 150, 150);

	/* Define el lander */
	rectangle(99, 99, 151, 151);

	setcolor(11);
	setfillstyle(9, 3);
	circle(125, 110, 10);
	floodfill(125, 110, 11);

	setcolor(10);
	setfillstyle(6, 2);
	rectangle(110, 121, 140, 135);
	floodfill(116, 122, 10);

	setcolor(15);
	line(115, 136, 110, 150);
	line(135, 136, 140, 150);

	setcolor(4);
	setfillstyle(9, 4);
	rectangle(120, 136, 130, 142);
	floodfill(121, 137, 4);

	Lander = malloc(Tamano);
	getimage(100, 100, 150, 150, Lander);

	LanderAnt = malloc(Tamano);
	getimage(100, 100, 150, 150, LanderAnt);

	/* Define el meteoro */
	cleardevice();
	setcolor(15);
	rectangle(99, 99, 151, 151);

	setfillstyle(5, 7);
	fillellipse(125, 125, 25, 15);
	floodfill(125, 125, 15);

	Meteoro = malloc(Tamano);
	getimage(100, 100, 150, 150, Meteoro);

	MeteoroAnt = malloc(Tamano);
	getimage(100, 100, 150, 150, MeteoroAnt);

	/* Define explo1 */
	cleardevice();
	setcolor(15);
	rectangle(99, 99, 151, 151);

	setcolor(12);
	setfillstyle(11, 4);
	circle(125, 125, 25);
	floodfill(125, 125, 12);

	Explo1 = malloc(Tamano);
	getimage(100, 100, 150, 150, Explo1);

	/* Define explo2 */
	cleardevice();
	setcolor(15);
	rectangle(99, 99, 151, 151);

	setcolor(14);
	setfillstyle(11, 6);
	circle(125, 125, 15);
	floodfill(125, 125, 14);

	Explo2 = malloc(Tamano);
	getimage(100, 100, 150, 150, Explo2);
}

/* Funci¢n para dibujar la pantalla */
void DibujaPantalla(void)
{
	/* Declaraci¢n de variables */
	int Posx, Posy, Color, yAct, xAnt, yAnt;

	/* Inicia el generador de n£meros aleatorios */
	randomize();

	/* Borra la pantalla */
	cleardevice();

	/* Dibuja las estrellas */
	for (int Contador = 0; Contador <= 1000; Contador++)
		{
			Posx = random(getmaxx());
			Posy = random(getmaxy());
			Color = random(getmaxcolor());
			putpixel(Posx, Posy, Color);
		}

	/* Dibuja el suelo lunar */
	xAnt = 0;
	yAnt = 400;
	setcolor(15);
	for (Contador = 30; Contador <= getmaxx(); Contador = Contador + 30)
		{
		 yAct = random(50) + 380;
		 line(xAnt, yAnt, Contador, yAct);
		 xAnt = Contador;
		 yAnt = yAct;
		}
	line(xAnt, yAnt, getmaxx(), 400);

	putpixel(0, getmaxy(), 0);
	setfillstyle(9, 15);
	floodfill(0, getmaxy(), 15);

	/* Dibuja la plataforma lunar */
	setcolor(14);
	setfillstyle(6, 6);
	rectangle(500, 420, 600, 470);
	floodfill(501, 421, 14);
}

/* Funci¢n que inicia el juego */
void IniciarJuego(void)
{
	xLander = 50;
	yLander = 50;
	xLanderAnt = xLander;
	yLanderAnt = yLander;
	xMeteoro = getmaxx();
	yMeteoro = random(getmaxy());
	xMeteoroAnt = xMeteoro;
	yMeteoroAnt = yMeteoro;

	getimage(xLander, yLander, xLander + 50, yLander + 50, LanderAnt);
	putimage(xLander, yLander, Lander, 1);
}

/* Funci¢n de proceso del programa */
void Proceso(void)
{
	/* Declaraci¢n de funciones a llamar */
	void MueveLander(void);
	void ExplotaLander(void);
	void Victoria(void);
	void MueveMeteoro(void);

	/* Variables locales */
	int Salir = Falso;
	int Tecla;

	while (Salir == Falso)
		{
			/* Obtiene una pulsaci¢n del teclado */
			if (kbhit() == -1)
				Tecla = getch();
			else
				Tecla = 0;

			/* Eval£a lo ingresado */
			if (Tecla == 27)
				Salir = Verdadero;
			if (Tecla == 75)
				{
					xLander = xLander - 5;
					if (xLander < 0)
						xLander = 0;
					MueveLander();
				}
			if (Tecla == 77)
				{
					xLander = xLander + 5;
					if (xLander > getmaxx())
						xLander = getmaxx();
					MueveLander();
				}
			if (Tecla == 72)
				{
					yLander = yLander - 5;
					if (yLander < 0)
						yLander = 0;
					MueveLander();
				}
			if (Tecla == 80)
				{
					yLander = yLander + 5;
					if (yLander > getmaxy())
						yLander = getmaxy();
					MueveLander();
				}

			/* Efecto de la gravedad */
			yLander = yLander + random(2);
			MueveLander();

			/* Mueve el meteoro */
			MueveMeteoro();

			/* Evaluaci¢n de eventos */
			if (yLander > 420) /* Si choca con el suelo lunar */
				{
					ExplotaLander();
					Salir = Verdadero;
				}
			/* Si el meteoro choca con el Lander */
			if ((xMeteoro >= xLander && xMeteoro <= xLander + 50) && (yMeteoro >= yLander && yMeteoro <= yLander + 50))
				{
					ExplotaLander();
					Salir = Verdadero;
				}
			if ((xMeteoro + 50 >= xLander && xMeteoro + 50 <= xLander + 50) && (yMeteoro + 50 >= yLander && yMeteoro + 50 <= yLander + 50))
				{
					ExplotaLander();
					Salir = Verdadero;
				}
			if ((yLander >= 380) && (xLander >= 500) && (xLander <= 600)) /* Si aterriza correctamente */
				{
					Victoria();
					Salir = Verdadero;
				}
		}
}

/* Funci¢n que mueve el lander */
void MueveLander(void)
{
	putimage(xLanderAnt, yLanderAnt, LanderAnt, 0);

	getimage(xLander, yLander, xLander + 50, yLander + 50, LanderAnt);
	putimage(xLander, yLander, Lander, 1);

	xLanderAnt = xLander;
	yLanderAnt = yLander;
}

/* Funci¢n que explosiona el Lander */
void ExplotaLander(void)
{
	for (int Contador = 1; Contador <= 5; Contador++)
		{
			putimage(xLander, yLander, Explo1, 0);
			delay(250);
			putimage(xLander, yLander, Explo2, 0);
			delay(250);
		}
}

/* Funci¢n que muestra la victoria */
void Victoria(void)
{
	setcolor(12);
	settextstyle(3, 0, 3);
	outtextxy(100, 100, "­­­Lo hiciste guacho!!!");
	delay(2000);
	getch();
}

/* Mueve Meteoro */
void MueveMeteoro(void)
{
	xMeteoro = xMeteoro - 10;
	if (xMeteoro < 0)
		{
			xMeteoro = getmaxx();
			yMeteoro = random(getmaxy());
		}
	putimage(xMeteoroAnt, yMeteoroAnt, MeteoroAnt, 0);

	getimage(xMeteoro, yMeteoro, xMeteoro + 50, yMeteoro + 50, MeteoroAnt);
	putimage(xMeteoro, yMeteoro, Meteoro, 1);

	xMeteoroAnt = xMeteoro;
	yMeteoroAnt = yMeteoro;

}

/* Funci¢n para finalizar el programa */
void FinPrograma(void)
{
	/* Declaraci¢n de funciones a llamar */
	void FinGrafico(void);
	setcolor(14);
	settextstyle(0, 0, 1);
	outtextxy((getmaxx() / 2) - 130, (getmaxy() / 2), "Presione una tecla para continuar");
	getch();
	FinGrafico();
}

/* Funci¢n para iniciar el modo gr fico */
int IniciaGrafico(void)
{
	/* Auto detecta el video */
	int gdriver = DETECT, gmode, CodigoError;

	/* Inicializa el modo grafico */
	initgraph(&gdriver, &gmode, "");

	/* Almacena el resultado de la inicializaci¢n */
	CodigoError = graphresult();

	/* Verifica si hubo error al iniciar el video */
	if (CodigoError == grOk)
		return(Verdadero);
	else
		return(Falso);
}

/* Funci¢n para finalizar el modo gr fico */
void FinGrafico(void)
{
	closegraph();
}

/* Funci¢n que despliega un mensaje de error */
void VerError(char Mensaje[80])
{
	clrscr();
	gotoxy(1,24);
	printf(Mensaje,"\n");
	getch();
}