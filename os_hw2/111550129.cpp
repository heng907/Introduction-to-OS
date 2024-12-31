/*
Student No.: 111550129
Student Name: 林彥亨
Email: yanheng.cs11@nycu.edu.tw
SE tag: xnxcxtxuxoxsx
Statement: I am fully aware that this program is not 
supposed to be posted to a public server, such as a 
public GitHub repository or a public web page.
*/

# include <bits/stdc++.h>
# include <unistd.h>
# include <sys/ipc.h>
# include <sys/shm.h>
# include <sys/wait.h>
# include <sys/time.h>

using namespace std;


// initialize a and b
void initializeMatrix(int* matrix, int dim) 
{
    for (int i = 0; i < dim * dim; i++) 
    {
        matrix[i] = i;
    }
}

// calculate checksum
unsigned int calculateChecksum(int* matrix, int dim) 
{
    unsigned int checksum = 0;
    for (int i = 0; i < dim * dim; i++) 
    {
        checksum += matrix[i];
    }
    return checksum;
}

int main()
{
    int dim; //100 - 800
    cout << "Input the matrix dimension: ";
    cin >> dim;
    cout << endl;

    // shared mem A, B, C
    int shm_id_A = shmget(IPC_PRIVATE, sizeof(int) * dim * dim, IPC_CREAT | 0600);
    int shm_id_B = shmget(IPC_PRIVATE, sizeof(int) * dim * dim, IPC_CREAT | 0600);
    int shm_id_C = shmget(IPC_PRIVATE, sizeof(int) * dim * dim, IPC_CREAT | 0600);


    if (shm_id_A == -1 || shm_id_B == -1 || shm_id_C == -1) 
    {
        perror("shmget failed");
        exit(1);
    }

    int *mem_A = (int *)shmat(shm_id_A, NULL, 0);
    int *mem_B = (int *)shmat(shm_id_B, NULL, 0);
    int *mem_C = (int *)shmat(shm_id_C, NULL, 0);

    if (mem_A == (void *)-1 || mem_B == (void *)-1 || mem_C == (void *)-1) 
    {
        perror("shmat failed");
        exit(1);
    }

    // init a, b
    initializeMatrix(mem_A, dim);
    initializeMatrix(mem_B, dim);

    // execute
    for(int process = 1; process <= 16 ; process++)
    {
        // initialize c
        for(int i = 0; i < dim * dim; i++)
        {
            mem_C[i] = 0;
        }

        struct timeval start, end;
        gettimeofday(&start, 0);

        int sub = dim / process;
        int remain = dim % process; 

        // matrix multiplication
        for(int p = 0; p < process; p++)
        {
            int col_to_exec = sub + (p < remain);
            int current_col = sub * p + (min(p, remain));

            pid_t pid = fork();
            if(pid == 0) // child
            {
                
                int *A = (int *)shmat(shm_id_A, NULL, 0);
                int *B = (int *)shmat(shm_id_B, NULL, 0);
                int *C = (int *)shmat(shm_id_C, NULL, 0);
                
                if (A == (void *)-1 || B == (void *)-1 || C == (void *)-1) 
                {
                    perror("Child shmat failed");
                    exit(1);
                }

                // C[i][j] = A[i][k] * B[k][j]
                for(int i = 0; i < dim; i++) 
                { 
                    for(int j = current_col; j < current_col + col_to_exec; j++) 
                    {
                        for(int k = 0; k < dim; k++) 
                        {
                            C[i * dim + j] += A[i * dim + k] * B[k * dim + j];
                        }
                    }
                }

                
                shmdt(A);
                shmdt(B);
                shmdt(C);
                exit(0);
            }
            else if (pid < 0)
            {
                cerr << "fork failed" << endl;
                exit(-1);
            }
        }
        
        
        for(int i = 0; i < process; i++)
        {
            wait(NULL);
        }

        // c's checksum
        unsigned int checksum = calculateChecksum(mem_C, dim);

        gettimeofday(&end, 0);
        int sec = end.tv_sec - start.tv_sec;
        int usec = end.tv_usec - start.tv_usec;

        cout << "Multiplying matrices using " << process << ((process == 1) ? (" process") : (" processes"))
             << "\nElapsed time: " << sec + (usec / 1000000.0) << " sec, Checksum: " << checksum << endl;
    }

    
    shmdt(mem_A);
    shmdt(mem_B);
    shmdt(mem_C);

    shmctl(shm_id_A, IPC_RMID, NULL);
    shmctl(shm_id_B, IPC_RMID, NULL);
    shmctl(shm_id_C, IPC_RMID, NULL);

    return 0;
}



        
        