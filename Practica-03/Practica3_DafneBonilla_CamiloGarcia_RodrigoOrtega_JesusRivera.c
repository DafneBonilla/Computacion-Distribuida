#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>

#define TAG_INAC 0
#define TAG_ELEC 1
#define TAG_RESP 2
#define TAG_VICT 3
#define TAG_ACTI 4

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

    // Checamos que el numero de nodos inactivos no sea menor a 1
    if (i < 1)
    {
        printf("El numero de nodos inactivos no puede ser menor a 1\n");
        MPI_Finalize();
        return 0;
    }

    // Inicializar el generador de numeros aleatorios con una semilla diferente para cada proceso
    time_t t;
    srand(time(&t) + rank);

    // Rango del nodo lider
    int lider = size;

    // Booleano para saber si el nodo esta activo o no
    int activo = 1;

    // Numero de nodos inactivos
    int inac = i;

    // El rango del nodo que inicia la eleccion (el primer nodo activo)
    int eleccion = 0;

    // El nodo 0 genera numeros aleatorios para los nodos inactivos y les avisa a todos los nodos si estan activos o no
    if (rank == 0)
    {
        int nodosActivos[size];
        for (int i = 0; i < size; i++)
        {
            nodosActivos[i] = 1;
        }
        int inactivos = 0;
        while (inactivos < inac)
        {
            int nodo = rand() % size;
            if (nodosActivos[nodo] == 1)
            {
                nodosActivos[nodo] = 0;
                inactivos++;
            }
        }
        for (int i = 0; i < size; i++)
        {
            MPI_Send(&nodosActivos[i], 1, MPI_INT, i, TAG_INAC, MPI_COMM_WORLD);
        }
        // Checamos cual es el primer nodo activo y lo enviamos a todos los nodos
        for (int i = 0; i < size; i++)
        {
            if (nodosActivos[i] == 1)
            {
                eleccion = i;
                break;
            }
        }
        for (int i = 0; i < size; i++)
        {
            MPI_Send(&eleccion, 1, MPI_INT, i, TAG_ACTI, MPI_COMM_WORLD);
        }
    }

    // Cada nodo recibe si esta activo o no
    MPI_Recv(&activo, 1, MPI_INT, 0, TAG_INAC, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Cada nodo recibe el rango del nodo que inicia la eleccion
    MPI_Recv(&eleccion, 1, MPI_INT, 0, TAG_ACTI, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

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

    // Hacemos que el primer nodo activo inicie la eleccion
    if (rank == eleccion)
    {
        for (int i = rank + 1; i < size; i++)
        {
            MPI_Send(&rank, 1, MPI_INT, i, TAG_ELEC, MPI_COMM_WORLD);
        }
        // TODO: esperar respuestas y luego ponerse a esperar mensajes como los demas nodos
    }
    else
    {
        // Los demas nodos esperan a recibir cualquier mensaje
        int esperando = 1;
        while (esperando != 0)
        {
            MPI_Status estado;
            int emisor;
            MPI_Recv(&emisor, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &estado);
            switch (estado.MPI_TAG)
            {
            // Al recibir un mensaje de eleccion, el nodo responde con un mensaje de respuesta
            case 1:
                // Si el nodo esta activo, iniciamos una eleccion
                if (activo == 1)
                {
                    MPI_Send(&rank, 1, MPI_INT, emisor, TAG_RESP, MPI_COMM_WORLD);
                    for (int i = rank + 1; i < size; i++)
                    {
                        MPI_Send(&rank, 1, MPI_INT, i, TAG_ELEC, MPI_COMM_WORLD);
                    }
                    // Esperamos a que todos los nodos respondan y checamos si somos el nodo activo con el rango mas alto
                    int respuestas = 0;
                    for (int i = rank + 1; i < size; i++)
                    {
                        int resp;
                        MPI_Recv(&resp, 1, MPI_INT, i, TAG_RESP, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        if (resp != -1)
                        {
                            respuestas++;
                        }
                    }
                    // Si somos el nodo activo con el rango mas alto, enviamos un mensaje de victoria a todos los nodos
                    if (respuestas == 0)
                    {
                        for (int i = 0; i < size; i++)
                        {
                            MPI_Send(&rank, 1, MPI_INT, i, TAG_VICT, MPI_COMM_WORLD);
                        }
                    }
                }
                // Si el nodo no esta activo, responde con un mensaje de respuesta negativa
                else
                {
                    int resp = -1;
                    MPI_Send(&resp, 1, MPI_INT, emisor, TAG_RESP, MPI_COMM_WORLD);
                }
                break;
            // Al recibir un mensaje de victoria, si el nodo esta activo, actualiza el rango del nodo lider y deja de esperar mensajes sin importar si esta activo o no
            case 3:
                if (activo == 1)
                {
                    lider = emisor;
                }
                esperando = 0;
                break;
            // En cualquier otro caso, el nodo sigue esperando mensajes
            default:
                break;
            }
        }
    }

    // Cada nodo muestra si esta activo o no, y cual es el rank del nodo lider
    if (rank == 0)
    {
        printf("Resultado:\n");
    }
    for (int i = 0; i < size; i++)
    {
        MPI_Barrier(MPI_COMM_WORLD);
        if (i == rank)
        {
            if (activo == 1)
            {
                printf("Nodo %d activo", rank);
            }
            else
            {
                printf("Nodo %d inactivo", rank);
            }
            printf(", con lider al nodo %d\n", lider);
        }
    }

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