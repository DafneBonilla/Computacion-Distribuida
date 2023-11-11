#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <string.h>

#define TAG_IND_INI 0
#define TAG_IND_FIN 1
#define TAG_FRAC_ORG 2
#define TAG_FRAC_ORD 3

// Funcion merge
int *merge(int a1[], int t1, int a2[], int t2)
{
    int *merged = (int *)malloc((t1 + t2) * sizeof(int));
    int x = 0;
    int y = 0;
    int z = 0;

    while (x < t1 && y < t2)
    {
        if (a1[x] < a2[y])
        {
            merged[z] = a1[x];
            x++;
        }
        else
        {
            merged[z] = a2[y];
            y++;
        }
        z++;
    }

    while (x < t1)
    {
        merged[z] = a1[x];
        x++;
        z++;
    }

    while (y < t2)
    {
        merged[z] = a2[y];
        y++;
        z++;
    }

    return merged;
}

// Funcion merge sort
void mergeSort(int arr[], int l, int r)
{
    if (l < r)
    {
        // Calculamos el indice del elemento que esta en la mitad del arreglo
        int m = l + (r - l) / 2;

        // Ordenamos la primera y la segunda mitad del arreglo
        mergeSort(arr, l, m);
        mergeSort(arr, m + 1, r);

        // Mezclamos las mitades ordenadas
        int *aux = merge(&arr[l], m - l + 1, &arr[m + 1], r - m);
        for (int i = 0; i < r - l + 1; i++)
        {
            arr[l + i] = aux[i];
        }
        free(aux);
    }
}

int main(int argc, char *argv[])
{
    int size, rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Checamos que se haya ingresado el numero de nodos y tamaño del arreglo
    if (argc != 3)
    {
        printf("Ingrese el numero de nodos y el tamaño del arreglo\n");
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

    // Checamos que el tamaño del arreglo sea mayor al numero de nodos
    if (i < n)
    {
        printf("El tamaño del arreglo debe ser mayor al numero de nodos\n");
        MPI_Finalize();
        return 0;
    }

    // Estrategia de estrella

    // Inicializar el generador de numeros aleatorios con una semilla diferente para cada proceso
    time_t t;
    srand(time(&t) + rank);

    // Arreglo original
    int original[i];

    // El nodo 0 genera un arreglo de numeros aleatorios (entre 0 y 99) y lo muestra
    if (rank == 0)
    {
        for (int j = 0; j < i; j++)
        {
            original[j] = rand() % 100;
        }
        printf("Arreglo original: ");
        for (int j = 0; j < i; j++)
        {
            printf("%d ", original[j]);
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);

    // Fraccion del arreglo que le corresponde a cada nodo
    int inicio = 0;
    int fin = i;

    // Arreglo auxiliar para guardar la fraccion del arreglo que le corresponde a cada nodo
    int fracciones[size][2];

    // El nodo 0 calcula el tamaño de la fraccion del arreglo que le corresponde a cada nodo
    if (rank == 0)
    {
        int tamanio = i / n;
        int sobra = i % n;
        for (int j = 0; j < size; j++)
        {
            fracciones[j][0] = inicio;
            fracciones[j][1] = inicio + tamanio - 1;
            if (sobra > 0)
            {
                fracciones[j][1]++;
                sobra--;
            }
            inicio = fracciones[j][1] + 1;
        }
    }

    // El nodo 0 envia a cada nodo de que indice a que indice le corresponde ordenar, los demas nodos reciben
    for (int j = 0; j < size; j++)
    {
        if (rank == 0)
        {
            if (j != 0)
            {
                MPI_Send(&fracciones[j][0], 1, MPI_INT, j, TAG_IND_INI, MPI_COMM_WORLD);
                MPI_Send(&fracciones[j][1], 1, MPI_INT, j, TAG_IND_FIN, MPI_COMM_WORLD);
            }
            else
            {
                inicio = fracciones[j][0];
                fin = fracciones[j][1];
            }
        }
        else
        {
            if (j == rank)
            {
                MPI_Recv(&inicio, 1, MPI_INT, 0, TAG_IND_INI, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&fin, 1, MPI_INT, 0, TAG_IND_FIN, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        }
    }

    // Cada nodo calcula el tamaño de la fraccion del arreglo que le corresponde ordenar
    int tamord = fin - inicio + 1;

    // Fraccion del arreglo
    int fraccion[tamord];

    // El nodo 0 reparte la fraccion del arreglo que le corresponde a cada nodo y los demas nodos reciben
    if (rank == 0)
    {
        for (int j = 0; j < size; j++)
        {
            if (j != 0)
            {
                int entregar = fracciones[j][1] - fracciones[j][0] + 1;
                MPI_Send(&original[fracciones[j][0]], entregar, MPI_INT, j, TAG_FRAC_ORG, MPI_COMM_WORLD);
            }
            else
            {
                for (int k = 0; k < tamord; k++)
                {
                    fraccion[k] = original[k];
                }
            }
        }
    }
    else
    {
        MPI_Recv(&fraccion, tamord, MPI_INT, 0, TAG_FRAC_ORG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // Cada nodo ordena su fraccion del arreglo usando merge sort
    mergeSort(fraccion, 0, tamord - 1);

    // Cada nodo envia su fraccion del arreglo ordenada al nodo 0
    for (int j = 0; j < size; j++)
    {
        if (rank != 0)
        {
            MPI_Send(&fraccion, tamord, MPI_INT, 0, TAG_FRAC_ORD, MPI_COMM_WORLD);
        }
    }

    // El nodo 0 va recibiendo las fracciones del arreglo ordenadas de cada nodo y las va uniendo
    if (rank == 0)
    {
        // Numero de elementos ordenados
        int ordenados = tamord;

        // Arreglo de elementos ordenados hasta el momento
        int *ordenado = (int *)malloc(ordenados * sizeof(int));

        // El nodo 0 guarda su fraccion del arreglo ordenada en el arreglo ordenado
        for (int j = 0; j < tamord; j++)
        {
            ordenado[j] = fraccion[j];
        }

        // El nodo 0 va recibe las fracciones
        for (int j = 1; j < size; j++)
        {
            int recibir = fracciones[j][1] - fracciones[j][0] + 1;
            int *aux = (int *)malloc(recibir * sizeof(int));
            MPI_Recv(aux, recibir, MPI_INT, j, TAG_FRAC_ORD, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            // Realiza el merge y actualiza el tamaño total de elementos ordenados
            ordenado = merge(ordenado, ordenados, aux, recibir);
            ordenados += recibir;

            // Liberar la memoria del arreglo auxiliar
            free(aux);
        }

        // Mostrar el arreglo ordenado
        printf("\nArreglo ordenado: ");
        for (int j = 0; j < ordenados; j++)
        {
            printf("%d ", ordenado[j]);
        }
        printf("\n");

        // Liberar la memoria del arreglo ordenado
        free(ordenado);
    }

    MPI_Finalize();
    return 0;
}

/*
Para compilar desde /Practica4:
mpicc Practica4_DafneBonilla_CamiloGarcia_RodrigoOrtega_JesusRivera.c
Para ejecutar desde /Practica4:
mpirun -np n --oversubscribe ./a.out n i
Donde n es el numero de nodos deseados, i es tamaño del arreglo
*/