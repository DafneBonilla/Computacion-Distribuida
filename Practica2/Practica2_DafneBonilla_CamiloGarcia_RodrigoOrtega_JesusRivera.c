#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>

#define TAG_IMPOSTOR 0
#define TAG_NUM_SUS 1

// Funcion para contar el numero de generales que atacan y se retiran
void mayoria(int planes[], int size, int *atacan, int *retiran)
{
    *atacan = 0;
    *retiran = 0;

    for (int i = 0; i < size; i++)
    {
        if (planes[i] == 1)
        {
            (*atacan)++;
        }
        else
        {
            (*retiran)++;
        }
    }
}

int main(int argc, char *argv[])
{
    int size, rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Checamos que se haya ingresado el numero de generales
    if (argc != 2)
    {
        printf("Ingrese el numero de generales\n");
        MPI_Finalize();
        return 0;
    }
    int n = atoi(argv[1]);

    // Checamos que el numero de generales sea igual al numero de procesos
    if (n != size)
    {
        printf("El numero de generales debe ser igual al numero de procesos\n");
        MPI_Finalize();
        return 0;
    }

    // Checamos que el numero de generales sea mayor a 0
    if (n < 1)
    {
        printf("El numero de generales debe ser mayor a 0\n");
        MPI_Finalize();
        return 0;
    }

    // Checamos que el numero de generales no sea 1
    if (n == 1)
    {
        printf("No se puede tener un solo general\n");
        MPI_Finalize();
        return 0;
    }

    // Inicializar el generador de numeros aleatorios con una semilla diferente para cada proceso.
    time_t t;
    srand(time(&t) + rank);

    // Booleanos para saber si el general es leal o impostor
    int leal = 1;

    // Numero de impostores
    int sus = 0;

    // Decidimos cuantos generales seran impostores, buscamos que el numero de generales sea (4t + 1) donde t es el numero de posibles impostores
    if (rank == 0)
    {
        if (size < 5)
        {
            sus = 1;
        }
        else
        {
            sus = (size - 1) / 4;
        }
    }

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

    // El general 0 envia a todos los generales el numero de impostores
    if (rank == 0)
    {
        for (int i = 0; i < size; i++)
        {
            MPI_Send(&sus, 1, MPI_INT, i, TAG_NUM_SUS, MPI_COMM_WORLD);
        }
    }

    // Recibimos el numero de impostores
    MPI_Recv(&sus, 1, MPI_INT, 0, TAG_NUM_SUS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Arreglo para los planes de cada general
    int planes[size];

    // Numero de generales que atacan y se retiran
    int ataca, retira;

    // Ponemos todos los planes como 0
    for (int i = 0; i < size; i++)
    {
        planes[i] = 0;
    }

    // Cada general decide si atacar o retirarse (atacar = 1, retirarse = 0) y lo guarda en su arreglo de planes
    planes[rank] = rand() % 2;

    // Proceso de decision
    // TODO: Hacer el algoritmo del rey

    // Cada general dice si es leal o impostor y su plan
    for (int i = 0; i < size; i++)
    {
        MPI_Barrier(MPI_COMM_WORLD);
        if (rank == i)
        {
            if (leal == 1)
            {
                printf("General %d: Soy leal y mi plan es %d\n", rank, planes[rank]);
            }
            else
            {
                printf("General %d: Soy impostor y mi plan es %d\n", rank, planes[rank]);
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
mpirun -np n --oversubscribe ./a.out n
Donde n es el numero de generales deseados
*/