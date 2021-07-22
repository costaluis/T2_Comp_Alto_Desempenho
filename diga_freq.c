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
#include <mpi.h>
#include <string.h>
#include <omp.h>

//Definição de constantes
#define TAM_LINE 1002
#define NUM_CHARS 96
#define T 2
#define NUMBER_LINES_MAX 200000

//Estrutura de dados responsável por armazenar frequência de caracteres
typedef struct charfreq
{
    int code, freq;
} charfreq;

//Lista responsável por armazenar as linhas junto de seus respectivos vetores de frequência
typedef struct list
{
    charfreq vector[NUM_CHARS];
    char line[TAM_LINE];
} list;


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
    int tam = strlen(elem->line);

    // Cria duas threads para processar a linha, cada thread receberá metade do tamanho da linha
    #pragma omp parallel for num_threads(T) 
    for (int i = 0; i < tam; i++){
        // Indica que a operação de soma deve ser feita de forma atômica, evitando que uma posição
        // seja alterada por duas threads ao mesmo tempo
        #pragma omp atomic
        elem->vector[elem->line[i] - 32].freq++;
    }
    // Ordena o vetor de frequências
    qsort(elem->vector, NUM_CHARS, sizeof(charfreq), compare);
}

//Função principal
int main(int argc, char * argv[])
{
    int provided, rank, size, i, ret, root=0;

    // Inicia o MPI indicando a utilização de threads com OMP
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    // Se não estiver disponível a utilização de múltiplas threads, encerra o processo
    if(provided != MPI_THREAD_MULTIPLE){
        printf("Problema na criacao das threads\n");
        return 1;
    }
    
    // Obtém o rank do processo e o size
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    //Declaração de variáveis
    list *prev;
    charfreq *vec_freq;
    list *lines_list;
    list * lines_vector;
    list * aux;

    int lines_number, tam, x;

    int rec_size, rec_size_resto;
    int *vetor_sizes, *vetor_dsp;
    list *vetor_env, *vetor_rec;

    double time;

     // Cria um tipo MPI para a struct charfreq 
    const int nitems_char_freq=2;
    int blocklenghts_char_freq[2] = {1,1};
    MPI_Datatype types_char_freq[2] = {MPI_INT, MPI_INT};
    MPI_Datatype mpi_char_freq;
    MPI_Aint     offsets_char_freq[2];

    offsets_char_freq[0] = offsetof(charfreq, code);
    offsets_char_freq[1] = offsetof(charfreq, freq);

    MPI_Type_create_struct(nitems_char_freq, blocklenghts_char_freq, offsets_char_freq, types_char_freq, &mpi_char_freq);
    MPI_Type_commit(&mpi_char_freq);

    // Cria um tipo MPI para a struct charfreq 
    const int nitems_list=2;
    int blocklenghts_list[2] = {NUM_CHARS,TAM_LINE};
    MPI_Datatype types_list[2] = {mpi_char_freq, MPI_CHAR};
    MPI_Datatype mpi_list;
    MPI_Aint     offsets_list[2];

    offsets_list[0] = offsetof(list, vector);
    offsets_list[1] = offsetof(list, line);

    MPI_Type_create_struct(nitems_list, blocklenghts_list, offsets_list, types_list, &mpi_list);
    MPI_Type_commit(&mpi_list);

    // Aloca o vetor de linhas e frequências que serão lidas e calculadas
    lines_list = (list*) malloc(NUMBER_LINES_MAX * sizeof(list));

    // Apenas o processo 0 executa esse trecho
    if(rank == 0){

        //Verificação de alocação
        if (lines_list == NULL)
        {
            printf("Falha na alocacao de memoria\n");
            return 1;
        }
        
        // Ininia o número de linhas lidas em 0
        lines_number = 0;


        //Rotina de leitura das linhas
        //Realiza leitura até que encontre EOF
        while ((fgets(lines_list[lines_number].line, TAM_LINE, stdin)))
        {
            // Verifica e remove o '\n' do fim da linha, caso exista
            x = strcspn(lines_list[lines_number].line, "\n");
            if(x != strlen(lines_list[lines_number].line)){
                lines_list[lines_number].line[x-1] = 0;
            }

            //Seta os códigos e a frequência inicial dos caracteres
            for (int i = 0; i < NUM_CHARS; i++)
            {
                lines_list[lines_number].vector[i].code = i + 32;
                lines_list[lines_number].vector[i].freq = 0;
            }

            // Incrementa um no número de linhas
            lines_number++;

        }

    }

    // Processo 0 inicia a contagem do tempo
    if(rank == 0){
        time = omp_get_wtime();
    }

    // Realiza um broadcast com o número de linhas lidas
    MPI_Bcast(&lines_number, 1, MPI_INT, 0, MPI_COMM_WORLD);
  
    // Cálculo do tamanho dos vetores de recebimento
    // Caso não número de linhas seja não múltiplo do número de processos
    // o resto é alocado para o último processo
    rec_size = lines_number / size;
    rec_size_resto= rec_size + lines_number % size;

    // Aloca os vetores de recebimento, offsets e tamanhos
    vetor_rec = (list*) malloc((rec_size_resto)*sizeof(list));
    vetor_dsp = (int*) malloc(size*sizeof(int));
    vetor_sizes= (int*) malloc(size*sizeof(int));
    
    // Atribui os valores calculados para os vetores
    for(i=0;i<(size-1);i++){
        vetor_sizes[i]=rec_size; 
        vetor_dsp[i]=i*rec_size; 
    }
    vetor_sizes[size-1]=rec_size_resto;
    vetor_dsp[size-1]=i*rec_size;

    // Realiza o espalhamento das linhas lidas para os processos
    MPI_Scatterv (lines_list,vetor_sizes,vetor_dsp,mpi_list,vetor_rec,rec_size_resto,mpi_list,root,MPI_COMM_WORLD);

    // Compara o processo que está, já que o último possui mais linhas para processar
    // Para cada linha recebida
        // Chama função de cálculo das frequências
    if(rank != (size-1)){
        for(int i=0; i<rec_size; i++){
            count_freq(&(vetor_rec[i]));
        }

    }else{
        for(int i=0; i<rec_size_resto; i++){
            count_freq(&(vetor_rec[i]));
        }
        rec_size = rec_size_resto;
    }

    // Reune todos as linhas processadas por cada processo em um único vetor do processo 0
    MPI_Gatherv(vetor_rec,rec_size,mpi_list,lines_list,vetor_sizes,vetor_dsp,mpi_list,root,MPI_COMM_WORLD);
    
    if(rank == 0){
        // Processamento terminado, coleta de tempo gasto e impressão
        time = omp_get_wtime() - time;

        // Print da saída
        for(int i=0; i<lines_number; i++){
            for(int j=0; j<NUM_CHARS; j++){
                printf("%d %d\n", lines_list[i].vector[j].code, lines_list[i].vector[j].freq);
            }
            printf("\n");
        }

        // Impressão do tempo
        //printf("Tempo gasto: %.10lf\n",time);
    }

    
    //Desaloca a memória utilizada
    free(lines_list);
    free(vetor_rec);
    free(vetor_dsp);
    free(vetor_sizes);

    // Finaliza os processos
    ret = MPI_Finalize();

    return(0);

}