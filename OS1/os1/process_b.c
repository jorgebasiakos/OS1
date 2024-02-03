#define TEXT_SZ 15

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <semaphore.h>
#include <time.h>
#include <pthread.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/mman.h>

struct shared_mem {
    char text_from_a[TEXT_SZ];
    char text_from_b[TEXT_SZ];
	int text_from_a_size;
	int text_from_b_size;
	int packet_from_a_complete;
	int packet_from_b_complete;
}*shared_data;

char* buffer_to_a;
size_t buffer_to_a_size = BUFSIZ;
char* buffer_from_a;
size_t buffer_from_a_size = BUFSIZ;

sem_t* sem_inter_b;
sem_t* sem_a_to_b_mutex;
sem_t* sem_a_to_b_empty;
sem_t* sem_a_to_b_full;
sem_t* sem_b_to_a_mutex;
sem_t* sem_b_to_a_empty;
sem_t* sem_b_to_a_full;

void* ta_function(void* arg);
void* tb_function(void* arg);

bool flag = true;			//for first entry
int out = 0;
int in = 0;
int running = 1;
pthread_t a_thread;
pthread_t b_thread;

int parts_num_out = 0;      //packets if > 15 to a
int parts_num_in = 0;       //packets if > 15 from a

clock_t start_time, end_time;
double total_time = 0.0;
bool flag_time = true; 

int main(){	
	buffer_from_a = malloc(BUFSIZ);
	buffer_to_a = malloc(BUFSIZ);

    int fd;
    char *shmpath = "e";		//like key

    fd = shm_open(shmpath, O_RDWR, 0);    if (fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(fd, sizeof(struct shared_mem)) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    shared_data = mmap(NULL, sizeof(struct shared_mem), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

	sem_unlink("sem_inter_b");
	sem_inter_b = sem_open("sem_inter_b", O_CREAT | O_EXCL, 0666, 1);
    if (sem_inter_b == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

	sem_a_to_b_mutex = sem_open("sem_a_to_b_mutex", O_EXCL);
    if (sem_a_to_b_mutex == SEM_FAILED) {
        perror("sem_open1");
        exit(EXIT_FAILURE);
    }	
	sem_a_to_b_full = sem_open("sem_a_to_b_full",O_EXCL);
	if (sem_a_to_b_full == SEM_FAILED) {
        perror("sem_open2");
        exit(EXIT_FAILURE);
    }	
	sem_a_to_b_empty = sem_open("sem_a_to_b_empty",O_EXCL);
	if (sem_a_to_b_empty == SEM_FAILED) {
        perror("sem_open3");
        exit(EXIT_FAILURE);
    }	

	sem_b_to_a_mutex = sem_open("sem_b_to_a_mutex", O_EXCL);
    if (sem_b_to_a_mutex == SEM_FAILED) {
        perror("sem_open4");
        exit(EXIT_FAILURE);
    }	
	sem_b_to_a_full = sem_open("sem_b_to_a_full",O_EXCL);
	if (sem_b_to_a_full == SEM_FAILED) {
        perror("sem_open5");
        exit(EXIT_FAILURE);
    }	
	sem_b_to_a_empty = sem_open("sem_b_to_a_empty",O_EXCL);
	if (sem_b_to_a_empty == SEM_FAILED) {
        perror("sem_open6");
        exit(EXIT_FAILURE);
    }	

    printf("Shared memory segment from \"%s\" has been created at \"%p\"\n", shmpath, (void *)shared_data);

	int res1, res2;
    void* thread_result1;
	void* thread_result2;

	res1 = pthread_create(&a_thread, NULL, ta_function, NULL);
	if(res1 != 0){
        perror("pthread_create");
        exit(EXIT_FAILURE);
	}
	res2 = pthread_create(&b_thread, NULL, tb_function, NULL);
	if(res2 != 0){
        perror("pthread_create");
        exit(EXIT_FAILURE);
	}
	
	res1 = pthread_join(a_thread, &thread_result1);
	if(res1 != 0){
        perror("pthread_create");
        exit(EXIT_FAILURE);
	}
	res2 = pthread_join(b_thread, &thread_result2);
	if(res2 != 0){
        perror("pthread_create");
        exit(EXIT_FAILURE);
	}

    if(munmap(shared_data, sizeof(struct shared_mem)) == -1){
        perror("munmap");
        exit(EXIT_FAILURE);
    }

    if (close(fd) == -1) {
        perror("close");
        exit(EXIT_FAILURE);
    }

	if (sem_close(sem_inter_b) == -1){
		perror("sem_close");
        exit(EXIT_FAILURE);
	}	
	if (sem_close(sem_a_to_b_mutex) == -1){
		perror("sem_close");
        exit(EXIT_FAILURE);
	}
	if (sem_close(sem_a_to_b_full) == -1){
		perror("sem_close");
        exit(EXIT_FAILURE);
	}
	if (sem_close(sem_a_to_b_empty) == -1){
		perror("sem_close");
        exit(EXIT_FAILURE);
	}
	if (sem_close(sem_b_to_a_mutex) == -1){
		perror("sem_close");
        exit(EXIT_FAILURE);
	}	
	if (sem_close(sem_b_to_a_full) == -1){
		perror("sem_close");
        exit(EXIT_FAILURE);
	}	
	if (sem_close(sem_b_to_a_empty) == -1){
		perror("sem_close");
        exit(EXIT_FAILURE);
	}

	if (sem_destroy(sem_inter_b) == -1){
		perror("sem_destroy");
        exit(EXIT_FAILURE);
	}	
	if (sem_destroy(sem_a_to_b_mutex) == -1){
		perror("sem_destroy");
        exit(EXIT_FAILURE);
	}
	if (sem_destroy(sem_a_to_b_full) == -1){
		perror("sem_destroy");
        exit(EXIT_FAILURE);
	}
	if (sem_destroy(sem_a_to_b_empty) == -1){
		perror("sem_destroy");
        exit(EXIT_FAILURE);
	}
	if (sem_destroy(sem_b_to_a_mutex) == -1){
		perror("sem_destroy");
        exit(EXIT_FAILURE);
	}	
	if (sem_destroy(sem_b_to_a_full) == -1){
		perror("sem_destroy");
        exit(EXIT_FAILURE);
	}	
	if (sem_destroy(sem_b_to_a_empty) == -1){
		perror("sem_destroy");
        exit(EXIT_FAILURE);
	}	

	printf("Total messages of Process_b = %d\n", out);
	printf("Total messages of Process_a = %d\n", in);
    printf("Total extra packets of Process_b = %d\n", parts_num_out);
    printf("Total extra packets of Process_a = %d\n", parts_num_in);
    printf("Avg extra packets from both Processes = %f\n", (float)(parts_num_out + parts_num_in)/ (out + in));
    if(parts_num_out != 0){
        printf("Avg time (b->a) for delivering packets = %f\n", total_time/parts_num_out);
    }

	return 0;
}

void* ta_function(void* arg){

	while(1) {
		sem_wait(sem_inter_b);
		if(running == 0){
			sem_post(sem_inter_b);
			//close other thread
			pthread_cancel(b_thread);
			pthread_exit(NULL);
		}
		sem_post(sem_inter_b);

		if(flag){
			printf("\nTyping here or at Process_a! If you give: '#BYE#' => END\n\n");
			flag = false;
		}

		int chars_sent = 0;
		int chars_read = getline(&buffer_to_a, &buffer_to_a_size, stdin);

		if (strncmp(buffer_to_a, "#BYE#", 5) == 0) {
			sem_wait(sem_inter_b);
			printf("Exiting..\n");
			running = 0;
			sem_post(sem_inter_b);
		}		

		while(chars_sent < chars_read){
			//producer-consumer
			sem_wait(sem_b_to_a_empty);
			sem_wait(sem_b_to_a_mutex);

			//break text inot packets
			if(chars_read - chars_sent > TEXT_SZ){
                if(flag_time){
                    start_time = clock();
                    flag_time = false;
                }
				shared_data->text_from_b_size = TEXT_SZ;
				shared_data->packet_from_b_complete = 0;
                parts_num_out++;    
            }
			else if(chars_read - chars_sent == TEXT_SZ){
				shared_data->text_from_b_size = TEXT_SZ;
				shared_data->packet_from_b_complete = 1;
			}
			else{
				shared_data->text_from_b_size = chars_read - chars_sent;
				shared_data->packet_from_b_complete = 1;
			}

			strncpy(shared_data->text_from_b, buffer_to_a+chars_sent, shared_data->text_from_b_size);

			chars_sent += shared_data->text_from_b_size;

			sem_post(sem_b_to_a_mutex);
			sem_post(sem_b_to_a_full);
		}
		out++;
        end_time = clock();
        total_time = total_time + ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
        flag_time = true;
	}
}

void* tb_function(void* arg){
	
	int chars_received = 0;
	while(1){
		sem_wait(sem_inter_b);
		if(running == 0){
			sem_post(sem_inter_b);	
			//close other thread
			pthread_cancel(a_thread);
			pthread_exit(NULL);
		}
		sem_post(sem_inter_b);	

		sem_wait(sem_a_to_b_full);
		sem_wait(sem_a_to_b_mutex);

		if(strncmp(shared_data->text_from_a, "#BYE#", 5) == 0){	
			sem_wait(sem_inter_b);
			running = 0;
			sem_post(sem_inter_b);												
		}												

		if(shared_data->packet_from_a_complete){
			//copy packet in whole buffer
			strncpy(buffer_from_a+chars_received, shared_data->text_from_a, shared_data->text_from_a_size );
			chars_received = 0;
	
			printf("Process_a wrote: %s\n", buffer_from_a);
			memset(buffer_from_a, 0, BUFSIZ);

			in++;
		}
		else{
			strncpy(buffer_from_a+chars_received, shared_data->text_from_a, shared_data->text_from_a_size );
			chars_received += shared_data->text_from_a_size;
            parts_num_in++;
		}

		sem_post(sem_a_to_b_mutex);
		sem_post(sem_a_to_b_empty);

	}
}