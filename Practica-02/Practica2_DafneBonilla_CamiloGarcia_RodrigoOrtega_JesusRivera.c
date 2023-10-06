#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>

#define TAG_IMPOSTOR 0
#define TAG_PLAN 1
#define TAG_REY 2

int main(int argc, char *argv[])
{
    int size, rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Checamos que se haya ingresado el numero de generales e impostores
    if (argc != 3)
    {
        printf("Ingrese el numero de generales e impostores\n");
        MPI_Finalize();
        return 0;
    }
    int n = atoi(argv[1]);
    int i = atoi(argv[2]);

    // Checamos que el numero de generales sea igual al numero de procesos
    if (n != size)
    {
        printf("El numero de generales debe ser igual al numero de procesos\n");
        MPI_Finalize();
        return 0;
    }

    // Checamos que el numero de generales sea mayor a 1
    if (n < 2)
    {
        printf("El numero de generales debe ser mayor a 1\n");
        MPI_Finalize();
        return 0;
    }

    // Checamos que el numero de impostores sea menor al numero de generales
    if (i >= n)
    {
        printf("El numero de impostores debe ser menor al numero de generales\n");
        MPI_Finalize();
        return 0;
    }

    // Checamos que el numero de impostores no sea menor a 1
    if (i < 1)
    {
        printf("El numero de impostores no puede ser menor a 1\n");
        MPI_Finalize();
        return 0;
    }

    // Inicializar el generador de numeros aleatorios con una semilla diferente para cada proceso
    time_t t;
    srand(time(&t) + rank);

    // Booleanos para saber si el general es leal o impostor
    int leal = 1;

    // Numero de impostores
    int sus = i;

    // Elegimos a los impostores y le avisamos a cada general si es impostor o no
    if (rank == 0)
    {
        int impostor[size];
        for (int i = 0; i < size; i++)
        {
            impostor[i] = 1;
        }
        for (int i = 0; i < sus; i++)
        {
            int imp = rand() % size;
            if (impostor[imp] == 1)
            {
                impostor[imp] = 0;
            }
            else
            {
                i--;
            }
        }
        for (int i = 0; i < size; i++)
        {
            MPI_Send(&impostor[i], 1, MPI_INT, i, TAG_IMPOSTOR, MPI_COMM_WORLD);
        }
    }

    // Recibimos si somos impostores o no
    MPI_Recv(&leal, 1, MPI_INT, 0, TAG_IMPOSTOR, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Arreglo para los planes de cada general
    int planes[size];

    // Ponemos todos los planes como 0
    for (int i = 0; i < size; i++)
    {
        planes[i] = 0;
    }

    // Cada general decide si atacar o retirarse (atacar = 1, retirarse = 0) y lo guarda en su arreglo de planes
    planes[rank] = rand() % 2;

    // Borrar esto luego de las pruebas /////////////////////////////////////////////////////////////////////
    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == 0)
    {
        printf("Planes de primera decision:\n");
    }
    for (int i = 0; i < size; i++)
    {
        MPI_Barrier(MPI_COMM_WORLD);
        if (i == rank)
        {
            printf("General %d con plan %d\n", rank, planes[rank]);
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == 0)
    {
        printf("\n");
    }
    /////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Decision de la mayoria
    int mayoria = 0;

    // Cantidad de la mayoria
    int cantidad_mayoria = 0;

    // Cantidad de votos necesarios para la mayoria, debe ser mayor a (generales / 2) + impostores
    int necesarios = (size / 2) + sus;

    // Numero del rank del rey
    int rey = 0;

    // Ronda actual
    int ronda = 1;

    // Numero de rondas, debe ser igual a (impostores + 1)
    int rondas = sus + 1;

    // Lo de abajo se repite el numero de rondas

    while (ronda <= rondas)
    {

        // Todos los generales actualizan la semilla del generador de numeros aleatorios para que el rey sea el mismo
        srand(time(&t) + ronda);

        // Se decide quien es el rey
        rey = rand() % size;

        // Los generales consiguen nuevas semillas para el generador de numeros aleatorios
        time_t t2;
        srand(time(&t2) + rank);

        // El general i envia su plan al general j, el general j recibe el plan y lo guarda
        for (int i = 0; i < size; i++)
        {
            for (int j = 0; j < size; j++)
            {
                if (i == rank)
                {
                    if (j != rank)
                    {
                        if (leal == 1)
                        {
                            MPI_Send(&planes[i], 1, MPI_INT, j, TAG_PLAN, MPI_COMM_WORLD);
                        }
                        else
                        {
                            int plan_falso = rand() % 2;
                            MPI_Send(&plan_falso, 1, MPI_INT, j, TAG_PLAN, MPI_COMM_WORLD);
                        }
                    }
                }
                else if (j == rank)
                {
                    if (i != rank)
                    {
                        MPI_Recv(&planes[i], 1, MPI_INT, i, TAG_PLAN, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    }
                }
            }
        }

        // Cada general cuenta planes y ve su mayoria
        int votos_ataca = 0;
        for (int i = 0; i < size; i++)
        {
            if (planes[i] == 1)
            {
                votos_ataca++;
            }
        }
        int votos_retira = size - votos_ataca;
        if (votos_ataca > votos_retira)
        {
            mayoria = 1;
            cantidad_mayoria = votos_ataca;
        }
        else
        {
            mayoria = 0;
            cantidad_mayoria = votos_retira;
        }

        // Borrar esto luego de las pruebas /////////////////////////////////////////////////////////////////////
        MPI_Barrier(MPI_COMM_WORLD);
        if (rank == 0)
        {
            printf("Arreglo de planes, ronda: %d\n", ronda);
        }
        MPI_Barrier(MPI_COMM_WORLD);
        for (int i = 0; i < size; i++)
        {
            MPI_Barrier(MPI_COMM_WORLD);
            if (i == rank)
            {
                printf("General %d con los planes: ", i);
                for (int j = 0; j < size; j++)
                {
                    printf("%d, ", planes[j]);
                }
                printf("\n");
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);
        if (rank == 0)
        {
            printf("\n");
        }
        /////////////////////////////////////////////////////////////////////////////////////////////////////////

        // El rey envia su mayoria a todos los generales, los generales reciben la mayoria y la guardan
        if (rank == rey)
        {
            for (int i = 0; i < size; i++)
            {
                if (leal == 1)
                {
                    MPI_Send(&mayoria, 1, MPI_INT, i, TAG_REY, MPI_COMM_WORLD);
                }
                else
                {
                    int mayoria_falsa = rand() % 2;
                    MPI_Send(&mayoria_falsa, 1, MPI_INT, i, TAG_REY, MPI_COMM_WORLD);
                }
            }
            // El rey guarda su mayoria
            if (leal == 1)
            {
                planes[rey] = mayoria;
            }
        }
        else
        {
            // Los demas generales reciben el plan del rey
            int mayoria_rey = 0;
            MPI_Recv(&mayoria_rey, 1, MPI_INT, rey, TAG_REY, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // Checamos si llegamos a un consenso, si no, guardamos el plan del rey
            if (leal == 1)
            {
                if (cantidad_mayoria > necesarios)
                {
                    planes[rank] = mayoria;
                }
                else
                {
                    planes[rank] = mayoria_rey;
                }
            }
        }
        // Borrar esto luego de las pruebas /////////////////////////////////////////////////////////////////////
        MPI_Barrier(MPI_COMM_WORLD);
        if (rank == 0)
        {
            printf("El rey es: %d\n", rey);
            printf("Arreglo de planes luego del rey, ronda: %d\n", ronda);
        }
        MPI_Barrier(MPI_COMM_WORLD);
        for (int i = 0; i < size; i++)
        {
            MPI_Barrier(MPI_COMM_WORLD);
            if (i == rank)
            {
                printf("General %d con los planes: ", i);
                for (int j = 0; j < size; j++)
                {
                    printf("%d, ", planes[j]);
                }
                printf("\n");
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);
        if (rank == 0)
        {
            printf("\n");
        }
        /////////////////////////////////////////////////////////////////////////////////////////////////////////
        ronda++;
    }

    // Repetimos lo de arriba

    // Vemos cual es nuestro plan final
    int plan_final = planes[rank];

    // Cada general dice si es leal o impostor y su plan
    if (rank == 0)
    {
        printf("Resultados:\n");
    }
    for (int i = 0; i < size; i++)
    {
        MPI_Barrier(MPI_COMM_WORLD);
        if (i == rank)
        {
            if (leal == 1)
            {
                printf("General %d: Leal, ", rank);
            }
            else
            {
                printf("General %d: Impostor, ", rank);
            }
            if (plan_final == 1)
            {
                printf("Ataca\n");
            }
            else
            {
                printf("Se retira\n");
            }
        }
    }

    MPI_Finalize();
    return 0;
}

/*
Para compilar desde /Practica1:
mpicc Practica2_DafneBonilla_CamiloGarcia_RodrigoOrtega_JesusRivera.c
Para ejecutar desde /Practica1:
mpirun -np n --oversubscribe ./a.out n i
Donde n es el numero de generales deseados, i es el numero de impostores deseados
*/