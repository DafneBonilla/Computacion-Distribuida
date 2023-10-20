#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>

#define TAG_LIDER 0

int main(int argc, char *argv[])
{
    int size, rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Checamos que se haya ingresado el numero de nodos y nodos inactivos
    if (argc != 3)
    {
        printf("Ingrese el numero de nodos y el numero de nodos inactivos\n");
        MPI_Finalize();
        return 0;
    }
    int n = atoi(argv[1]);
    int i = atoi(argv[2]);

    // Checamos que el numero de nodos sean iguales al numero de procesos
    if (n != size)
    {
        printf("El numero de nodos debe ser igual al numero de procesos\n");
        MPI_Finalize();
        return 0;
    }

    // Checamos que el numero de nodos sea mayor a 1
    if (n < 2)
    {
        printf("El numero de nodos debe ser mayor a 1\n");
        MPI_Finalize();
        return 0;
    }

    // Checamos que el numero de nodo inactivos sea menor al numero de nodos
    if (i >= n)
    {
        printf("El numero de nodos inactivos debe ser menor al numero de nodos\n");
        MPI_Finalize();
        return 0;
    }

    // Checamos que el numero de nodos inactivos sea mayor a 0
    if (i <= 0)
    {
        printf("El numero de nodos inactivos debe ser mayor a 0\n");
        MPI_Finalize();
        return 0;
    }

    // Inicializar el generador de numeros aleatorios con una semilla diferente para cada proceso
    time_t t;
    srand(time(&t) + rank);

    // Algortimo de eleccion, Algoritmo de Bully

    // Informacion: https://www.geeksforgeeks.org/bully-algorithm-in-distributed-system/ y wikipedia
    /*
    The algorithm uses the following message types:

    Election Message: Sent to announce election.
    Answer (Alive) Message: Responds to the Election message.
    Coordinator (Victory) Message: Sent by winner of the election to announce victory.
    When a process P recovers from failure, or the failure detector indicates that the current coordinator has failed, P performs the following actions:

    If P has the highest process ID, it sends a Victory message to all other processes and becomes the new Coordinator. Otherwise, P broadcasts an Election message to all other processes with higher process IDs than itself.
    If P receives no Answer after sending an Election message, then it broadcasts a Victory message to all other processes and becomes the Coordinator.
    If P receives an Answer from a process with a higher ID, it sends no further messages for this election and waits for a Victory message. (If there is no Victory message after a period of time, it restarts the process at the beginning.)
    If P receives an Election message from another process with a lower ID it sends an Answer message back and if it has not already started an election, it starts the election process at the beginning, by sending an Election message to higher-numbered processes.
    If P receives a Coordinator message, it treats the sender as the coordinator.
    */

    MPI_Finalize();
    return 0;
}

/*
Para compilar desde /Practica3:
mpicc Practica3_DafneBonilla_CamiloGarcia_RodrigoOrtega_JesusRivera.c
Para ejecutar desde /Practica3:
mpirun -np n --oversubscribe ./a.out n i
Donde n es el numero de nodos deseados, i es el numero de nodos inactivos deseados
*/