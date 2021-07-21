/*Trabalho 01 - Computação de Alto Desempenho

Grupo 01

João Pedro Fidelis Belluzzo - 10716661
Leonardo Cerce Guioto - 10716640
Luis Fernando Costa de Oliveira - 10716532 
Rodrigo Augusto Valeretto - 10684792
Thiago Daniel Cagnoni de Pauli - 10716629

*/

//Includes bibliotecas
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <mpi.h>
#include <string.h>

//Definição de constantes
#define TAM_LINE 1002
#define NUM_CHARS 96
#define T 8

//Estrutura de dados responsável por armazenar frequência de caracteres
typedef struct charfreq
{
    int code, freq;
} charfreq;

//Lista encadeada de linhas lidas e vetores de frequência
typedef struct list
{
    charfreq vector[96];
    char line[1000];
    struct list *next;
} list;

//Função de liberação de memória
void free_memory(list *aux)
{
    if (aux->next)
    {
        free_memory(aux->next);
    }
    free(aux);
}

//Função de comparação utilizada no qsort
int compare(const void *p1, const void *p2)
{
    charfreq *cf1 = (charfreq *)p1;
    charfreq *cf2 = (charfreq *)p2;

    if (cf1->freq != cf2->freq)
        return cf1->freq - cf2->freq;

    return cf2->code - cf1->code;
}

//Função responsável por realizar contagem do número de caracteres
void count_freq(list *elem)
{
    //int tam = strlen(elem->line);

    //#pragma omp simd 
    for (int i = 0; i < TAM_LINE; i++)
    {
        
        if (elem->line[i] == '\n' || elem->line[i] == '\0')
        {
            break;
        }
        
        elem->vector[elem->line[i] - 32].freq++;
    }

    qsort(elem->vector, NUM_CHARS, sizeof(charfreq), compare);
}

//Função principal
int main(int argc, char * argv[])
{
    //Declaração de variáveis
    list *prev;
    charfreq *vec_freq;
    list *lines_list = (list *)malloc(sizeof(list));
    list ** lines_vector;

    int lines_number=0;
    int rank, size, i;

    int rec_size, rec_size_resto;
    int *vetor_sizes, *vetor_dsp;
    list *vetor_env, *vetor_rec;

    //Verificação de alocação
    if (lines_list == NULL)
    {
        printf("Falha na alocacao de memoria\n");
        return 1;
    }
    list *aux = lines_list;

    /*
    char *line = (char *)malloc(TAM_LINE * sizeof(char));
    //Verificação de alocação
    if (line == NULL)
    {
        printf("Falha na alocacao de memoria\n");
        return 1;
    }
    */

    //Variável para realizar contagem do tempo de execução
    double time;

    //Rotina de leitura das linhas
    //Realiza leitura até que encontre EOF
    while (fgets(aux->line, TAM_LINE, stdin))
    {
        //Realiza alocação da linha lida em um elemento da lista encadeada
        //aux->line = line;

        /*
        //Aloca um vetor de frequências na memória
        vec_freq = (charfreq *)malloc(NUM_CHARS * sizeof(charfreq));

        //Verificação de alocação
        if (vec_freq == NULL)
        {
            printf("Falha na alocacao de memoria\n");
            exit(1);
        }
        */

        //Seta os códigos e a frequência inicial dos caracteres
        for (int i = 0; i < NUM_CHARS; i++)
        {
            aux->vector[i].code = i + 32;
            aux->vector[i].freq = 0;
        }

        //Aloca o vetor de frequências no elemento da lista encadeada
        //aux->vector = vec_freq;

        //Chamada da diretiva task
        //Define uma tarefa que será executada pelas threads disponíveis
        //Cada thread executará o cálculo de frequência de uma linha por vez

        //Aloca memória para o próximo nó da lista encadeada
        aux->next = (list *)malloc(sizeof(list));

        //Verificação de alocação
        if (aux->next == NULL)
        {
            printf("Falha na alocacao de memoria\n");
            exit(1);
        }

        //Armazena o nó anterior da lista encadeada
        prev = aux;

        //Avança o nó da lista encadeada
        aux = aux->next;

    }

    //Algoritmo chega aqui quando lê EOF
    //Desaloca o vetor de caracteres que não será utilizado
    //free(line);

    //Desaloca o nó que não será utilizado
    free(prev->next);

    //Seta o último nó da lista encadeada como NULL
    prev->next = NULL;

    aux = lines_list;

    while (aux!=NULL)
    {   
        printf("%s",aux->line);
        lines_number++;
        aux = aux->next;
    }
    
/*

    //omp_set_nested(1);

    //Início da região paralela
    #pragma omp parallel num_threads(T)
    {
        //Início da região single - Executado por uma única thread
        #pragma omp single
        {
            aux = lines_list;

            //Início da contagem de tempo
            time = omp_get_wtime();


            //Percorre a lista encadeada, criando uma task para cada nó
            //Cada nó possui uma linha e um vetor de frequência correspondente
            while(aux != NULL)
            {
                //Diretiva task com firsprivate para garantir acesso à região de memória pela task
                #pragma omp task firstprivate(aux)
                {
                //Chamada da função que será executada na task
                count_freq(aux);
                }
                aux = aux->next;
            }
            
        }
    }

    //Realiza a coleta de tempo de execução
    //O tempo de execução é medido sem considerar tempo de impressão
    time = omp_get_wtime() - time;

    //Seta a variável temporária de volta à cabeça da lista encadeada para percorrê-la
    aux = lines_list;

    //Enquanto a lista encadeada não acabou
    while (aux != NULL)
    {
        for (int i = 0; i < NUM_CHARS; i++)
        {
            //Realiza o print do código do caractere e sua frequência de aparição, caso ela não seja 0
            if (aux->vector[i].freq)
            {
                printf("%d %d\n", aux->vector[i].code, aux->vector[i].freq);
            }
        }

        //Avança para o próximo nó
        aux = aux->next;

        //Verifica se não é a ultima linha para realizar a quebra de linha
        if (aux)
        {
            printf("\n");
        }
    }

    //Impressão do tempo de execução
    printf("\n%.10lf\n", time);
    */
    //Chamada da rotina que libera a memória alocada na lista encadeada
    free_memory(lines_list);


}