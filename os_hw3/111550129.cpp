/*
Student No.: 111550129
Student Name: 林彥亨
Email: yanheng0907@gmail.com
SE tag: xnxcxtxuxoxsx
Statement: I am fully aware that this program is not 
supposed to be posted to a public server, such as a 
public GitHub repository or a public web page.
*/

#include <pthread.h>
#include <semaphore.h>
#include <bits/stdc++.h>
#include <sys/time.h>

using namespace std;

struct ThreadPool
{
    sem_t mutex, finish;
    int n;  //n threads
    int thread_num;
    int array[1000000]; //for input integers
    int done[16] = {0};
    int table[16][2] = {{0, 0}};
    pthread_t *threads;
    queue<int> work_list;


    //bubble sort
    void BubbleSort(int left, int right)
    {
        for(int i = left; i < right; i++)
        {
            for(int j = i + 1; j < right; j++)
            {
                if(array[i] > array[j])
                {
                    swap(array[i], array[j]);
                }
            }
        }
    }

    //merge
    void Merge(int begin, int mid, int end)
    {
        int Left = begin, Right = mid, index = 0;
        vector<int> temp(end - begin);

        while(Left < mid && Right < end)
        {
            if(array[Left] < array[Right])
            {
                temp[index++] = array[Left++];
            }
            else
            {
                temp[index++] = array[Right++];
            }
        }

        while(Left < mid) 
        {
            temp[index++] = array[Left++];
        }
        while(Right < end)
        {
            temp[index++] = array[Right++];
        }

        for(int i = 0; i < index; i++)
        {
            array[begin + i] = temp[i];
        }
    }

    static void *merge_sort(void *this_pool)
    {
        ThreadPool *p = static_cast<ThreadPool *>(this_pool);

        while (1)
        {
            sem_wait(&p->mutex);
            if(p->work_list.empty())
            {
                sem_post(&p->mutex);
                sem_post(&p->finish);
                return NULL;
            }

            int job = p->work_list.front();
            p->work_list.pop();
            sem_post(&p->mutex);

            if(job > 7)
            {
                p->table[job][0] = (job - 8) * (p->n / 8);
                p->table[job][1] = (job != 15) ? (p->table[job][0] + (p->n / 8)) : (p->n);
                p->BubbleSort(p->table[job][0], p->table[job][1]);
            }
            else
            {
                p->table[job][0] = p->table[job * 2][0];
                p->table[job][1] = p->table[job * 2 + 1][1];
                p->Merge(p->table[job][0], p->table[job*2][1], p->table[job][1]);
            }

            sem_wait(&p->mutex);
            p->done[job] = 1;
            if(p->done[job + ((job & 1) ? -1 : 1)])
            {
                p->work_list.push(job >> 1);
            }
            sem_post(&p->mutex);
        }
        
    }

    ThreadPool(int thread) : thread_num(thread){}

    void init()
    {
        ifstream fin("input.txt");
        fin >> n;
        for(int i = 0; i < n; i++)
        {
            fin >> array[i];
        }
        fin.close();

        sem_init(&mutex, 0, 1);
        sem_init(&finish, 0, 0);

        threads = new pthread_t[thread_num];

        for(int job = 8; job < 16; job++)
        {
            work_list.push(job);
        }

    }

    void exe()
    {
        for(int i = 0; i<thread_num; i++)
        {
            pthread_create(&threads[i], NULL, merge_sort, this);
        }

        for(int i = 0; i < thread_num; i++)
        {
            sem_wait(&finish);
        }
    }

    void write()
    {
        string filename  = "output_" + to_string(thread_num) + ".txt";
        ofstream fout(filename);
        for(int i = 0; i < n; i++)
        {
            fout << array[i] << " ";
        }
        fout.close();
    }
    
        void destroy() {
        delete[] threads;
        sem_destroy(&mutex);
        sem_destroy(&finish);
    }


};

int main()
{
    for(int thread = 1; thread < 9; thread++)
    {
        struct timeval start, stop;
        gettimeofday(&start, 0);

        ThreadPool pool(thread);
        pool.init();
        pool.exe();
        pool.write();

        gettimeofday(&stop, 0);
        int sec = stop.tv_sec - start.tv_sec;
        int usec = stop.tv_usec - start.tv_usec;
        cout << "worker thread #" << thread << ", elapsed " << (sec + (usec / 1000000.0)) * 1000.0 << " ms\n";
        pool.destroy();

    }
}