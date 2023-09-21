#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <string.h>

#define TAG_SOLICITUD 1
#define TAG_RESPUESTA 2
#define TAG_O_DISTANCIA 3
#define TAG_O_DISTANCIA_MOD 4
#define TAG_CAMBIO 5

int main(int argc, char *argv[])
{
    int size, rank;
    int send_msg = 123;
    int recv_msg;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Inicializar el generador de números aleatorios con una semilla diferente para cada proceso
    time_t t;
    srand(time(&t) + rank);

    // Arreglo para almacenar las distancias entre todos los nodos
    int distancia[size];
    for (int i = 0; i < size; i++)
    {
        distancia[i] = 0;
    }

    // checamos que el numero de nodos sea mayor a 1
    if (size < 2)
    {
        printf("La distancia del nodo 0 al nodo 0 es 0\n");
        MPI_Finalize();
        return 0;
    }

    // 1. Cada nodo obtiene su distancia inicial a todos los demas
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            if (rank == i)
            {
                if (rank != j)
                {
                    // El nodo i envia un mensaje al nodo j
                    MPI_Send(&send_msg, 1, MPI_INT, j, TAG_SOLICITUD, MPI_COMM_WORLD);
                    // El nodo i recibe la respuesta del nodo j
                    MPI_Recv(&recv_msg, 1, MPI_INT, j, TAG_RESPUESTA, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    // El nodo i almacena el mensaje recibido en su arreglo de distancias
                    distancia[j] = recv_msg;
                }
            }
            else if (rank == j)
            {
                // El nodo j recibe la solicitud del nodo i
                MPI_Recv(&recv_msg, 1, MPI_INT, i, TAG_SOLICITUD, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                // El nodo j genera un numero aleatorio entre 1 y 100
                recv_msg = rand() % 100 + 1;
                // El nodo j envia el numero aleatorio al nodo i
                MPI_Send(&recv_msg, 1, MPI_INT, i, TAG_RESPUESTA, MPI_COMM_WORLD);
            }
        }
    }

    // Arreglo de arreglos para almacenar la ruta mas corta de cada nodo
    int ruta[size][3];

    // Ponemos primero que la ruta mas corta al nodo i es 0, luego el nodo i y luego un -1 para indicar que no hay mas nodos
    for (int i = 0; i < size; i++)
    {
        ruta[i][0] = 0;
        ruta[i][1] = i;
        ruta[i][2] = -1;
    }

    // Arreglo para que cada nodo almacene el arreglo de distancias del nodo 0
    int distancia_cero[size];

    if (rank == 0)
    {
        // El nodo 0 copia su arreglo de distancias al arreglo de distancias del nodo 0
        for (int i = 0; i < size; i++)
        {
            distancia_cero[i] = distancia[i];
        }
    }

    // Booleano para saber si se modifico alguna distancia
    int cambio = 1;

    while (cambio)
    {

        // 2. El nodo 0 envia su arreglo de distancias a todos los demas nodos.
        for (int i = 0; i < size; i++)
        {
            if (rank == 0)
            {
                if (rank != i)
                {
                    // El nodo 0 envia su arreglo de distancias al nodo i
                    MPI_Send(&distancia_cero, size, MPI_INT, i, TAG_O_DISTANCIA, MPI_COMM_WORLD);
                }
            }
            else if (rank == i)
            {
                // El nodo i recibe el arreglo de distancias del nodo 0
                MPI_Recv(&distancia_cero, size, MPI_INT, 0, TAG_O_DISTANCIA, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        }

        // 3. Para cada entrada i del arreglo, el nodo j almacena el mínimo entre la distancia actual de 0 a i y la suma de la distancia de 0 a j + la distancia de j a i
        for (int i = 0; i < size; i++)
        {
            if (rank != 0)
            {
                int actual = distancia_cero[i];
                int nueva = distancia_cero[rank] + distancia[i];
                if (actual > nueva)
                {
                    distancia_cero[i] = nueva;
                }
            }
        }

        // 4. Los nodos regresan los arreglos modificados al nodo 0
        if (rank != 0)
        {
            MPI_Send(&distancia_cero, size, MPI_INT, 0, TAG_O_DISTANCIA_MOD, MPI_COMM_WORLD);
        }

        // Actualizamos el booleano de cambio
        if (rank == 0)
        {
            cambio = 0;
        }

        // 5. El nodo 0 los revisa uno por uno, almacenando el mínimo para cada entrada. En caso de un cambio entonces quitamos al ultimo del arreglo de arreglos y ponemos que pasamos por el nodo i
        for (int i = 0; i < size; i++)
        {
            if (rank == 0)
            {
                if (rank != i)
                {
                    int aux[size];
                    MPI_Recv(&aux, size, MPI_INT, i, TAG_O_DISTANCIA_MOD, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    for (int j = 0; j < size; j++)
                    {
                        int actual = distancia_cero[j];
                        int nueva = aux[j];
                        if (actual > nueva)
                        {
                            distancia_cero[j] = nueva;
                            // Si hay un cambio modificamos la ruta mas corta
                            ruta[j][0] = 0;
                            ruta[j][1] = i;
                            ruta[j][2] = j;
                            // Actualizamos el booleano de cambio
                            cambio = 1;
                        }
                    }
                }
            }
        }

        // El nodo 0 envia su booleano de cambio a todos los demas nodos
        for (int i = 0; i < size; i++)
        {
            if (rank == 0)
            {
                if (rank != i)
                {
                    MPI_Send(&cambio, 1, MPI_INT, i, TAG_CAMBIO, MPI_COMM_WORLD);
                }
            }
            else if (rank == i)
            {
                MPI_Recv(&cambio, 1, MPI_INT, 0, TAG_CAMBIO, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        }

        // 6. Repetir los pasos 2 a 5 hasta que no haya cambios en el arreglo del paso 5.
    }

    // 7. Mostrar el retraso total de la ruta del nodo 0 a todos los demas nodos y la grafica (el arreglo distancia de cada nodo)
    if (rank == 0)
    {
        printf("\n");
        // Mostramos la ruta mas corta de cada nodo
        for (int i = 0; i < size; i++)
        {
            printf("La distancia del nodo 0 al nodo %i es %i\n", i, distancia_cero[i]);

            // Cadena para almacenar el camino mas corto
            char cadena[1100];
            // Inicializamos la cadena como vacía
            cadena[0] = '\0';

            int booleano = 1;
            int actual = i;

            while (booleano)
            {
                if (ruta[actual][2] == -1)
                {
                    int aux = ruta[actual][1];
                    // Transformamos el numero a cadena
                    char temp[100];
                    sprintf(temp, " -> %d", aux);

                    // Concatenamos temp al principio de cadena
                    char temp2[1100];
                    sprintf(temp2, "%s%s", temp, cadena);
                    strcpy(cadena, temp2);

                    // Ponemos el booleano en 0 para salir del ciclo
                    booleano = 0;
                }
                else
                {
                    // Transformamos el numero a cadena
                    char temp[100];
                    sprintf(temp, " -> %d", ruta[actual][2]);

                    // Concatenamos temp al principio de cadena
                    char temp2[1100];
                    sprintf(temp2, "%s%s", temp, cadena);
                    strcpy(cadena, temp2);

                    actual = ruta[actual][1];
                }
            }
            printf("El camino mas corto es: 0%s\n", cadena);
        }
        printf("\n");
        printf("Grafica de distancias:\n");
    }
    // Todos esperan a que el nodo 0 termine de imprimir
    MPI_Barrier(MPI_COMM_WORLD);
    for (int i = 0; i < size; i++)
    {
        if (rank == i)
        {
            printf("Nodo %i: ", rank);
            for (int j = 0; j < size; j++)
            {
                printf("%i, ", distancia[j]);
            }
            printf("\n");
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == 0)
    {
        printf("\n");
    }

    MPI_Finalize();
    return 0;
}

/*
Para compilar desde /Practica1:
mpicc Practica1_DafneBonilla_CamiloGarcia_RodrigoOrtega_JesusRivera.c
Para ejecutar desde /Practica1:
mpirun -np n --oversubscribe ./a.out
Donde n es el numero de nodos deseados
*/