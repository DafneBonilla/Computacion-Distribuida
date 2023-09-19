#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>

#define TAG_SOLICITUD 1
#define TAG_RESPUESTA 2
#define TAG_O_DISTANCIA 3
#define TAG_O_DISTANCIA_MOD 4

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

    /* Prueba de que el arreglo de distancias se lleno correctamente
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
    */

    // Arreglo de arreglos para almacenar los nodos en el camino mas corto
    int lista[size][size + 1];

    // Ponemos primero que la distancia minima a cada nodo i pasa por el nodo i, el resto de los nodos los ponemos en 0 y el ultimo ponemos i
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size + 1; j++)
        {
            if (j == 0)
            {
                lista[i][j] = i;
            }
            else if (j == size)
            {
                lista[i][j] = i;
            }
            else
            {
                lista[i][j] = 0;
            }
        }
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

    // Numero de iteracion
    int iteracion = 0;

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

    /* Prueba de que los nodos tienen el arreglo de distancias del nodo 0
    for (int i = 0; i < size; i++)
    {
        if (rank == i)
        {
            if (rank == 0)
            {
                printf("Nodo %i: ", rank);
                for (int j = 0; j < size; j++)
                {
                    printf("%i, ", distancia[j]);
                }
                printf("\n");
            }
            else
            {
                printf("Nodo %i: ", rank);
                for (int j = 0; j < size; j++)
                {
                    printf("%i, ", distancia_cero[j]);
                }
                printf("\n");
            }
        }
    }
    */

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
                        lista[j][iteracion] = i;
                    }
                }
            }
        }
    }

    // 6. Repetir los pasos 2 a 5 hasta que no haya cambios en el arreglo del paso 5.

    // 7. Mostrar el retraso total de la ruta del nodo 0 a todos los demas nodos y la grafica (el arreglo distancia de cada nodo)
    if (rank == 0)
    {
        printf("\n");
        // Mostramos el retraso total de la ruta del nodo 0 a todos los demas nodos
        for (int i = 0; i < size; i++)
        {
            printf("La distancia del nodo 0 al nodo %i es %i\n", i, distancia_cero[i]);
            // Mostramos el camino mas corto (si encontramos muchos ceros los ignoramos)
            printf("El camino mas corto es: 0");
            for (int j = 0; j < size + 1; j++)
            {
                if (lista[i][j] != 0)
                {
                    printf(" -> %i", lista[i][j]);
                }
            }
            printf("\n");
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