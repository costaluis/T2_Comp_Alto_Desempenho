#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <stddef.h>

typedef struct car_s {
        int shifts[3];
        int topSpeed;
        char linha[10];
} car;

typedef struct charfreq
{
    int code, freq;
} charfreq;

typedef struct line_freq
{
    charfreq vector_freq[96];
    char line[1000];
    struct line_freq * next;
} line_freq;

int main(int argc, char **argv) {

    const int tag = 13;
    int size, rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        fprintf(stderr,"Requires at least two processes.\n");
        exit(-1);
    }

    /* create a type for struct car */
    const int nitems_char_freq=2;
    int          blocklenghts_char_freq[2] = {1,1};
    MPI_Datatype types_char_freq[2] = {MPI_INT, MPI_INT};
    MPI_Datatype mpi_char_freq;
    MPI_Aint     offsets_char_freq[2];

    offsets_char_freq[0] = offsetof(charfreq, code);
    offsets_char_freq[1] = offsetof(charfreq, freq);

    MPI_Type_create_struct(nitems_char_freq, blocklenghts_char_freq, offsets_char_freq, types_char_freq, &mpi_char_freq);
    MPI_Type_commit(&mpi_char_freq);

    /* create a type for struct car */
    const int nitems_line_freq=2;
    int          blocklenghts_line_freq[2] = {96,1000};
    MPI_Datatype types_line_freq[2] = {mpi_char_freq, MPI_CHAR};
    MPI_Datatype mpi_line_freq;
    MPI_Aint     offsets_line_freq[2];

    offsets_line_freq[0] = offsetof(line_freq, vector_freq);
    offsets_line_freq[1] = offsetof(line_freq, line);

    MPI_Type_create_struct(nitems_line_freq, blocklenghts_line_freq, offsets_line_freq, types_line_freq, &mpi_line_freq);
    MPI_Type_commit(&mpi_line_freq);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        line_freq send;

        for (int i = 0; i < 96; i++)
        {
            send.vector_freq[i].code = i + 32;
            send.vector_freq[i].freq = 0;
        }

        sprintf(send.line, "Isso eh um teste");

        const int dest = 1;
        MPI_Send(&send,   1, mpi_line_freq, dest, tag, MPI_COMM_WORLD);

        printf("Rank %d: sent line_freq\n", rank);
    }
    if (rank == 1) {
        MPI_Status status;
        const int src=0;

        line_freq recv;

        MPI_Recv(&recv,   1, mpi_line_freq, src, tag, MPI_COMM_WORLD, &status);
        printf("Char_code %d: Char_freq: %d Line: %s\n", recv.vector_freq[0].code,recv.vector_freq[0].freq, recv.line);
    }

    MPI_Type_free(&mpi_char_freq);
    MPI_Type_free(&mpi_line_freq);
    MPI_Finalize();

    return 0;
}
