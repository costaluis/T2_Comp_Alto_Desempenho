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

//Lista encadeada de linhas lidas e vetores de frequência
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

    #pragma omp parallel for num_threads(T) 
    for (int i = 0; i < tam; i++){
        #pragma omp atomic
        elem->vector[elem->line[i] - 32].freq++;
    }

    qsort(elem->vector, NUM_CHARS, sizeof(charfreq), compare);
}

//Função principal
int main(int argc, char * argv[])
{
    int provided, rank, size, i, ret, root=0;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    if(provided != MPI_THREAD_MULTIPLE){
        printf("Problema na criacao das threads\n");
        return 1;
    }
    
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    //Declaração de variáveis
    list *prev;
    charfreq *vec_freq;
    list *lines_list;
    list * lines_vector;
    list * aux;

    int lines_number, tam;

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

    lines_list = (list*) malloc(NUMBER_LINES_MAX * sizeof(list));

    if(rank == 0){

        //Verificação de alocação
        if (lines_list == NULL)
        {
            printf("Falha na alocacao de memoria\n");
            return 1;
        }

        //Variável para realizar contagem do tempo de execução
        

        lines_number = 0;


        //Rotina de leitura das linhas
        //Realiza leitura até que encontre EOF
        while ((fgets(lines_list[lines_number].line, TAM_LINE, stdin)))
        {
            //printf("Rank: %d Line: %s\n", rank, lines_list[lines_number].line);
            int x = strcspn(lines_list[lines_number].line, "\n");
            if(x != strlen(lines_list[lines_number].line)){
                lines_list[lines_number].line[x-1] = 0;
            }
            //Seta os códigos e a frequência inicial dos caracteres
            for (int i = 0; i < NUM_CHARS; i++)
            {
                lines_list[lines_number].vector[i].code = i + 32;
                lines_list[lines_number].vector[i].freq = 0;
            }

            //Chamada da diretiva task
            //Define uma tarefa que será executada pelas threads disponíveis
            //Cada thread executará o cálculo de frequência de uma linha por vez

            lines_number++;

        }

    }

    if(rank == 0){
        time = omp_get_wtime();
    }

    MPI_Bcast(&lines_number, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);

  
    rec_size = lines_number / size;
    rec_size_resto= rec_size + lines_number % size;

    vetor_rec = (list*) malloc((rec_size_resto)*sizeof(list));
    vetor_dsp = (int*) malloc(size*sizeof(int));
    vetor_sizes= (int*) malloc(size*sizeof(int));
    
  
    for(i=0;i<(size-1);i++){
        vetor_sizes[i]=rec_size; // amount of items in this slot
        vetor_dsp[i]=i*rec_size; // shift in relation to start address
    }
    vetor_sizes[size-1]=rec_size_resto;
    vetor_dsp[size-1]=i*rec_size;


    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Scatterv (lines_list,vetor_sizes,vetor_dsp,mpi_list,vetor_rec,rec_size_resto,mpi_list,root,MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);

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
    MPI_Barrier(MPI_COMM_WORLD);


    MPI_Gatherv(vetor_rec,rec_size,mpi_list,lines_list,vetor_sizes,vetor_dsp,mpi_list,root,MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
 
    
    if(rank == 0){
        for(int i=0; i<lines_number; i++){
            for(int j=0; j<NUM_CHARS; j++){
                printf("%d %d\n", lines_list[i].vector[j].code, lines_list[i].vector[j].freq);
            }
            printf("\n");
        }
    }
    

    if(rank==0){
        time = omp_get_wtime() - time;
        //printf("Tempo gasto: %.10lf",time);
    }

    ret = MPI_Finalize();

    //Chamada da rotina que libera a memória alocada na lista encadeada
    free(lines_list);
        
    return(0);

}