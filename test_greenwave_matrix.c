/* PMSIS includes */
#include "test_greenwave_matrix.h"
#include "pmsis.h"

/* Variables used. */
#define BUFFER_SIZE_PER_CORE ( 256 )



struct cl_args_s_final
{
    uint32_t size;
    
    uint16_t *l2_in1;
    uint16_t *l2_in2;
    
    uint16_t *l1_buffer1;
    uint16_t *l1_buffer2;

    uint32_t *l1_bufferAdd;
    uint32_t *l1_bufferMult;

    uint32_t *l2_outAdd;
    uint32_t *l2_outMult;
    uint32_t *l2_outConv;
};

PI_L2 static struct cl_args_s_final cl_arg;

void printMatrix16(uint16_t *ptr, uint32_t size)
{
    for(uint32_t i =0;i<size;i++){
        for(uint32_t j =0; j<size; j++){
            printf("%u\t", ptr[i*size+j]);
        }
        printf("\n");
    }
}

void printMatrix32(uint32_t *ptr, uint32_t size)
{
    for(uint32_t i =0;i<size;i++){
        for(uint32_t j =0; j<size; j++){
            printf("%u\t", ptr[i*size+j]);
        }
        printf("\n");
    }
}

/* Task executed by cluster cores. */
void cluster_matrix(void *arg)
{
    struct cl_args_s_final *dma_args = (struct cl_args_s_final *) arg;
    uint16_t *l1_buffer1 = dma_args->l1_buffer1;
    uint16_t *l1_buffer2 = dma_args->l1_buffer2;
    uint32_t *l1_bufferAdd = dma_args->l1_bufferAdd;
    uint32_t *l1_bufferMult = dma_args->l1_bufferMult;
    
    uint16_t *l2_in1 = dma_args->l2_in1;
    uint16_t *l2_in2 = dma_args->l2_in2;
    uint32_t *l2_outAdd = dma_args->l2_outAdd;
    uint32_t *l2_outMult = dma_args->l2_outMult;
    uint32_t *l2_outConv = dma_args->l2_outConv;

    uint32_t buffer_size = dma_args->size;

    uint32_t coreid = pi_core_id(), start = 0, end = 0;

    /* Core 0 of cluster initiates DMA transfer from L2 to L1 of first matrix. */
    if (!coreid)
    {
        printf("Core %d requesting DMA transfer from l2_in to l1_buffer.\n", coreid);
        pi_cl_dma_copy_t copy1;
        copy1.dir = PI_CL_DMA_DIR_EXT2LOC;
        copy1.merge = 0;
        copy1.size = buffer_size * 2;
        copy1.id = 0;
        copy1.ext = (uint32_t) l2_in1;
        copy1.loc = (uint32_t) l1_buffer1;

        pi_cl_dma_memcpy(&copy1);
        pi_cl_dma_wait(&copy1);
        printf("Core %d : Transfer done.\n", coreid);
    }

    /* Core 0 of cluster initiates DMA transfer from L2 to L1 of second matrix. */
    if (!coreid)
    {
        printf("Core %d requesting DMA transfer from l2_in to l1_buffer.\n", coreid);
        pi_cl_dma_copy_t copy1;
        copy1.dir = PI_CL_DMA_DIR_EXT2LOC;
        copy1.merge = 0;
        copy1.size = buffer_size * 2;
        copy1.id = 0;
        copy1.ext = (uint32_t) l2_in2;
        copy1.loc = (uint32_t) l1_buffer2;

        pi_cl_dma_memcpy(&copy1);
        pi_cl_dma_wait(&copy1);
        printf("Core %d : Transfer done.\n", coreid);
    }

    start = (coreid * (buffer_size / pi_cl_cluster_nb_pe_cores()));
    if(buffer_size % 2 == 0)
        end = (start - 1 + (buffer_size / pi_cl_cluster_nb_pe_cores()));
    else
        end = (start + (buffer_size / pi_cl_cluster_nb_pe_cores()));

    /* Barrier synchronisation before starting to compute. */
    pi_cl_team_barrier(0);
    /* Each core computes on specific portion of buffer. */
    for (uint32_t i=start; i<=end; i++)
    {
        l1_bufferAdd[i] = (uint32_t)(l1_buffer1[i] + l1_buffer2[i]);
        l1_bufferMult[i] = (uint32_t)(l1_buffer1[i] * l1_buffer2[i]);
    }
    /* Barrier synchronisation to wait for all cores. */
    pi_cl_team_barrier(0);

    /* Core 0 of cluster initiates DMA transfer from L1 to L2. */
    if (!coreid)
    {
        printf("Core %d requesting DMA transfer from l1_buffer to l2_out.\n", coreid);
        pi_cl_dma_copy_t copy;
        copy.dir = PI_CL_DMA_DIR_LOC2EXT;
        copy.merge = 0;
        copy.size = buffer_size * 4;
        copy.id = 0;
        copy.ext = (uint32_t) l2_outAdd;
        copy.loc = (uint32_t) l1_bufferAdd;

        pi_cl_dma_memcpy(&copy);
        pi_cl_dma_wait(&copy);
        printf("Core %d : Transfer done.\n", coreid);
    }  
    /* Core 0 of cluster initiates DMA transfer from L1 to L2. */
    if (!coreid)
    {
        printf("Core %d requesting DMA transfer from l1_buffer to l2_out.\n", coreid);
        pi_cl_dma_copy_t copy;
        copy.dir = PI_CL_DMA_DIR_LOC2EXT;
        copy.merge = 0;
        copy.size = buffer_size * 4;
        copy.id = 0;
        copy.ext = (uint32_t) l2_outMult;
        copy.loc = (uint32_t) l1_bufferMult;

        pi_cl_dma_memcpy(&copy);
        pi_cl_dma_wait(&copy);
        printf("Core %d : Transfer done.\n", coreid);
    }
    /*convolution*/
    int size = 64;
    if (!coreid)
    {
        unsigned short filter[9] = {-1, -2, -1, 0, 0, 0, 1, 2, 1};
        for (int y = 0; y < (int)size; y++) {
            for (int x = 0; x < (int)size; x++) {
                uint32_t sum = 0;
                for (int i = -1; i <= 1; i++) {
                    for (int j = -1; j <= 1; j++) {
                        int posX = x + j;
                        int posY = y + i;

                        if (posX >= 0 && posX < (int)size && posY >= 0 && posY < (int)size) {
                            int filterIndex = (i + 1) * 3 + (j + 1);
                            sum += l1_bufferMult[posY * size + posX] * filter[filterIndex];
                        }
                    }
                }
                l1_bufferAdd[y * size + x] = sum;
            }
        }
        printf("Core %d requesting DMA transfer from l1_buffer to l2_out.\n", coreid);
        pi_cl_dma_copy_t copy;
        copy.dir = PI_CL_DMA_DIR_LOC2EXT;
        copy.merge = 0;
        copy.size = buffer_size * 4;
        copy.id = 0;
        copy.ext = (uint32_t) l2_outConv;
        copy.loc = (uint32_t) l1_bufferAdd;

        pi_cl_dma_memcpy(&copy);
        pi_cl_dma_wait(&copy);
        printf("Core %d : Transfer done.\n", coreid);
    }
}

/* Cluster main entry, executed by core 0. */
void master_entry(void *arg)
{
    printf("Cluster master core entry\n");
    /* Task dispatch to cluster cores. */
    pi_cl_team_fork(pi_cl_cluster_nb_pe_cores(), cluster_matrix, arg);
    printf("Cluster master core exit\n");
}

void test_matrix(void)
{
    printf("Entering main controller\n");

    uint32_t matrix_size = 64; //size in unsigned short

    uint32_t errors = 0;
    struct pi_device cluster_dev;
    struct pi_cluster_conf conf;

    uint32_t nb_cl_pe_cores = pi_cl_cluster_nb_pe_cores();
    uint32_t buffer_size = matrix_size * matrix_size; // uint16_t
    
    //allocate memory in l2
    uint16_t *l2_in1 = pi_l2_malloc(buffer_size * 2); // size in byte
    uint16_t *l2_in2 = pi_l2_malloc(buffer_size * 2); // size in byte
    uint32_t *l2_outAdd = pi_l2_malloc(buffer_size * 4); // size in byte
    uint32_t *l2_outMult = pi_l2_malloc(buffer_size * 4); // size in byte
    uint32_t *l2_outConv = pi_l2_malloc(buffer_size * 4); // size in byte
   
    if (l2_in1 == NULL || l2_in2 == NULL || l2_outAdd == NULL || l2_outMult == NULL || l2_outConv == NULL)
    {
        printf("Matrix alloc failed !\n");
        pmsis_exit(-1);
    }
  
    /* Init cluster configuration structure. */
    pi_cluster_conf_init(&conf);
    conf.id = 0;                /* Set cluster ID. */
    /* Configure & open cluster. */
    pi_open_from_conf(&cluster_dev, &conf);
    if (pi_cluster_open(&cluster_dev))
    {
        printf("Cluster open failed !\n");
        pmsis_exit(-3);
    }
    uint16_t *l1_buffer1 = pi_cl_l1_malloc(&cluster_dev, buffer_size * 2);
    uint16_t *l1_buffer2 = pi_cl_l1_malloc(&cluster_dev, buffer_size * 2);
    uint32_t *l1_bufferAdd = pi_cl_l1_malloc(&cluster_dev, buffer_size * 4);
    uint32_t *l1_bufferMult = pi_cl_l1_malloc(&cluster_dev, buffer_size * 4);

    if (l1_buffer1 == NULL || l1_buffer2 == NULL) 
    {
        printf("l1_buffer alloc failed !\n");
        pi_cluster_close(&cluster_dev);
        pmsis_exit(-4);
    }
    if (l1_bufferAdd == NULL )
    {
        printf("l1_results alloc failed !\n");
        pi_cluster_close(&cluster_dev);
        pmsis_exit(-4);
    }
    if (l1_bufferMult == NULL)
    {
        printf("l1_mult alloc failed !\n");
        pi_cluster_close(&cluster_dev);
        pmsis_exit(-4);
    }

    /* Matrix Init. */
    for (uint32_t i=0; i<buffer_size; i++)
    {
        l1_buffer1[i] = 0;
        l1_buffer2[i] = 0;
        l1_bufferAdd[i] = 0;
        l1_bufferMult[i] = 0;
        l2_in1[i] = i;
        l2_in2[i] = i;
        l2_outAdd[i] = 0;
        l2_outMult[i] = 0;
        l2_outConv[i] = 0;
    }
    /* Init arg struct. */
    cl_arg.size = buffer_size;
    cl_arg.l2_in1 = l2_in1;
    cl_arg.l2_in2 = l2_in2;
    cl_arg.l1_buffer1 = l1_buffer1;
    cl_arg.l1_buffer2 = l1_buffer2;
    cl_arg.l1_bufferAdd = l1_bufferAdd;
    cl_arg.l1_bufferMult = l1_bufferMult;
    cl_arg.l2_outAdd = l2_outAdd;
    cl_arg.l2_outMult = l2_outMult;
    cl_arg.l2_outConv = l2_outConv;

    /* Prepare cluster task and send it to cluster. */
    struct pi_cluster_task *task = pi_l2_malloc(sizeof(struct pi_cluster_task));
    if (task == NULL)
    {
        printf("Cluster task alloc failed !\n");
        pi_cluster_close(&cluster_dev);
        pmsis_exit(-5);
    }
    
    pi_cluster_task(task, master_entry, &cl_arg);

    printf("Sending task.\n");
    #if defined(ASYNC)
    pi_task_t wait_task;
    pi_task_block(&wait_task);
    pi_cluster_send_task_to_cl_async(&cluster_dev, task, &wait_task);
    printf("Wait end of cluster task\n");
    pi_task_wait_on(&wait_task);
    printf("End of cluster task\n");
    #else
    pi_cluster_send_task_to_cl(&cluster_dev, task);
    #endif  /* ASYNC */

    printf("\nMatrix l2_in1 :\n");
    printMatrix16(l2_in1, matrix_size);
    printf("\nMatrix l2_in2 :\n");
    printMatrix16(l2_in2, matrix_size);
    printf("\nMatrix l1_buffer1 :\n");
    printMatrix16(l1_buffer1, matrix_size);
    printf("\nMatrix l1_buffer2 :\n");
    printMatrix16(l1_buffer2, matrix_size);
    printf("\nMatrix l1_bufferAdd :\n");
    printMatrix32(l1_bufferAdd, matrix_size);
    printf("\nMatrix l1_bufferMult :\n");
    printMatrix32(l1_bufferMult, matrix_size);
    printf("\nMatrix l2_outAdd :\n");
    printMatrix32(l2_outAdd, matrix_size);
    printf("\nMatrix l2_outMult :\n");
    printMatrix32(l2_outMult, matrix_size);
    printf("\nMatrix l2_outConv :\n");
    printMatrix32(l2_outConv, matrix_size);

    pi_l2_free(task, sizeof(struct pi_cluster_task));
    pi_cl_l1_free(&cluster_dev, l1_buffer1, buffer_size);
    pi_cl_l1_free(&cluster_dev, l1_buffer2, buffer_size);
    pi_cl_l1_free(&cluster_dev, l1_bufferAdd, buffer_size);
    pi_cl_l1_free(&cluster_dev, l1_bufferMult, buffer_size);

    printf("Close cluster after end of computation.\n");
    pi_cluster_close(&cluster_dev);

    /* Verification. */
    /*for (uint32_t i=0; i<buffer_size; i++)
    {
        if (l2_out[i] != (uint8_t) (l2_in[i] * 3))
        {
            errors++;
            printf("%d) %x-%x ", i, l2_out[i], (uint8_t) (l2_in[i] * 3));
        }
    }*/

    pi_l2_free(l2_in1, buffer_size);
    pi_l2_free(l2_in2, buffer_size);
    pi_l2_free(l2_outAdd, buffer_size);
    pi_l2_free(l2_outMult, buffer_size);
    pi_l2_free(l2_outConv, buffer_size);

    printf("\nCluster DMA done with %d error(s) !\n", errors);

    pmsis_exit(errors);
}

/* Program Entry. */
int main(int argc, char* argv[])
{
    printf("\n\n\t *** Matrix test ***\n\n");
    return pmsis_kickoff((void *) test_matrix);
}
