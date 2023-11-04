#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>

#define TAG_INAC 0
#define TAG_ELEC 1
#define TAG_RESP 2
#define TAG_VICT 3
#define TAG_ACTI 4
#define TAG_TERM 5

// Funcion para ver si todos los nodos ya terminaron
int yaTermine(int rank, int size, int terminados[])
{
    for (int i = 0; i < size; i++)
    {
        if (terminados[i] == 0)
        {
            return 0;
        }
    }
    return 1;
}

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

    // Checamos que el numero de nodos inactivos no sea menor a 0
    if (i < 0)
    {
        printf("El numero de nodos inactivos no puede ser menor a 0\n");
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

    // Idea de que los nodos inactivos se elijan aleatoriamente y que el primer nodo activo inicie la eleccion
    // Fue descartada debido a la dificultad de poder hacer que siempre termine la ejecucion del programa, generado por la manera en que terminan los nodos
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
            if (i != rank)
            {
                MPI_Send(&nodosActivos[i], 1, MPI_INT, i, TAG_INAC, MPI_COMM_WORLD);
            }
        }
        activo = nodosActivos[rank];
        // Checamos cual es el primer nodo activo y lo enviamos a todos los nodos
        int pos = 0;
        for (int i = 0; i < size; i++)
        {
            if (nodosActivos[i] == 1)
            {
                pos = i;
                break;
            }
        }
        for (int i = 0; i < size; i++)
        {
            if (rank != i)
            {
                MPI_Send(&pos, 1, MPI_INT, i, TAG_ACTI, MPI_COMM_WORLD);
            }
        }
        eleccion = pos;
    }

    // Cada nodo recibe si esta activo o no
    if (rank != 0)
    {
        MPI_Recv(&activo, 1, MPI_INT, 0, TAG_INAC, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // Cada nodo recibe el primer nodo activo
    if (rank != 0)
    {
        MPI_Recv(&eleccion, 1, MPI_INT, 0, TAG_ACTI, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    /*
    // El nodo 0 avisa a todos los nodos si estan activos o no
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
            nodosActivos[inactivos] = 0;
            inactivos++;
        }
        for (int i = 0; i < size; i++)
        {
            if (i != rank)
            {
                MPI_Send(&nodosActivos[i], 1, MPI_INT, i, TAG_INAC, MPI_COMM_WORLD);
            }
        }
        activo = nodosActivos[rank];
    }

    // Cada nodo recibe si esta activo o no
    if (rank != 0)
    {
        MPI_Recv(&activo, 1, MPI_INT, 0, TAG_INAC, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    */

    // Arreglo para saber que nodos ya terminaron
    int terminados[size];

    // Cada nodo inicia el arreglo de nodos terminados
    for (int i = 0; i < size; i++)
    {
        terminados[i] = 0;
    }

    // Hacemos que el primer nodo activo inicie la eleccion (sera siempre el nodo 0)
    if (rank == eleccion)
    {
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
                if (i != rank)
                {
                    MPI_Send(&rank, 1, MPI_INT, i, TAG_VICT, MPI_COMM_WORLD);
                }
            }
            // Si el nodo esta activo, actualizamos el rango del nodo lider
            if (activo == 1)
            {
                lider = rank;
            }
            // Enviamos un mensaje que ya terminamos a todos los nodos
            for (int i = 0; i < size; i++)
            {
                if (i != rank)
                {
                    MPI_Send(&rank, 1, MPI_INT, i, TAG_TERM, MPI_COMM_WORLD);
                }
            }
            terminados[rank] = 1;
            // Esperamos a que todos los nodos terminen
            int esperando = yaTermine(rank, size, terminados);
            while (esperando != 0)
            {
                MPI_Status estado;
                int emisor;
                MPI_Recv(&emisor, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &estado);
                switch (estado.MPI_TAG)
                {
                // Al recibir un mensaje de terminar, marcamos que el emisor ya termino
                case 5:
                    terminados[emisor] = 1;
                    // Si todos los nodos menores al nodo actual ya terminaron, el nodo actual termina
                    if (yaTermine(rank, size, terminados) == 1)
                    {
                        esperando = 0;
                    }
                // En cualquier otro caso, el nodo sigue esperando mensajes
                default:
                    break;
                }
            }
        }
        // Si no somos el nodo activo con el rango mas alto, esperamos a recibir un mensaje de victoria
        else
        {
            int esperando = 1;
            while (esperando != 0)
            {
                MPI_Status estado;
                int emisor;
                MPI_Recv(&emisor, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &estado);
                switch (estado.MPI_TAG)
                {
                // Al recibir un mensaje de victoria, actualizamos el rango del nodo lider y dejamos de esperar mensajes
                case 3:
                    if (activo == 1)
                    {
                        lider = emisor;
                    }
                    // Enviamos un mensaje que ya terminamos a todos los nodos
                    for (int i = 0; i < size; i++)
                    {
                        if (i != rank)
                        {
                            MPI_Send(&rank, 1, MPI_INT, i, TAG_TERM, MPI_COMM_WORLD);
                        }
                    }
                    terminados[rank] = 1;
                    // Si todos los nodos ya terminaron, el nodo actual termina
                    if (yaTermine(rank, size, terminados) == 1)
                    {
                        esperando = 0;
                    }
                    break;
                // Al recibir un mensaje de terminar, marcamos que el emisor ya termino
                case 5:
                    terminados[emisor] = 1;
                    // Si todos los nodos menores al nodo actual ya terminaron, el nodo actual termina
                    if (yaTermine(rank, size, terminados) == 1)
                    {
                        esperando = 0;
                    }
                // En cualquier otro caso, el nodo sigue esperando mensajes
                default:
                    break;
                }
            }
        }
        // TODO: esperar respuestas y luego ponerse a esperar mensajes como los demas nodos
    }
    else
    {
        // Los demas nodos esperan a recibir cualquier mensaje
        int esperando = 1;
        int eleccion = 0;
        while (esperando != 0)
        {
            MPI_Status estado;
            int emisor;
            MPI_Recv(&emisor, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &estado);
            switch (estado.MPI_TAG)
            {
            // Al recibir un mensaje de eleccion, el nodo responde con un mensaje de respuesta
            case 1:
                // Si el nodo esta activo, iniciamos una eleccion si no hemos iniciado una antes
                if (activo == 1)
                {
                    MPI_Send(&rank, 1, MPI_INT, emisor, TAG_RESP, MPI_COMM_WORLD);
                    if (eleccion == 0)
                    {
                        eleccion = 1;
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
                                if (i != rank)
                                {
                                    MPI_Send(&rank, 1, MPI_INT, i, TAG_VICT, MPI_COMM_WORLD);
                                }
                            }
                            // Si el nodo esta activo, actualizamos el rango del nodo lider
                            if (activo == 1)
                            {
                                lider = rank;
                            }
                            // Enviamos un mensaje que ya terminamos a todos los nodos
                            for (int i = 0; i < size; i++)
                            {
                                if (i != rank)
                                {
                                    MPI_Send(&rank, 1, MPI_INT, i, TAG_TERM, MPI_COMM_WORLD);
                                }
                            }
                            terminados[rank] = 1;
                            // Si todos los nodos ya terminaron, el nodo actual termina
                            if (yaTermine(rank, size, terminados) == 1)
                            {
                                esperando = 0;
                            }
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
            // Al recibir un mensaje de victoria, si el nodo esta activo, actualiza el rango del nodo lider
            case 3:
                if (activo == 1)
                {
                    lider = emisor;
                }
                // Enviamos un mensaje que ya terminamos a todos los nodos
                for (int i = 0; i < size; i++)
                {
                    if (i != rank)
                    {
                        MPI_Send(&rank, 1, MPI_INT, i, TAG_TERM, MPI_COMM_WORLD);
                    }
                }
                terminados[rank] = 1;
                // Si todos los nodos ya terminaron, el nodo actual termina
                if (yaTermine(rank, size, terminados) == 1)
                {
                    esperando = 0;
                }
                break;
            // Al recibir un mensaje de terminar, marcamos que el emisor ya termino
            case 5:
                terminados[emisor] = 1;
                // Si todos los nodos ya terminaron, el nodo actual termina
                if (yaTermine(rank, size, terminados) == 1)
                {
                    esperando = 0;
                }
            // En cualquier otro caso, el nodo sigue esperando mensajes
            default:
                break;
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
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