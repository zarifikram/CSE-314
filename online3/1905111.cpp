#include<bits/stdc++.h>
#include<iostream>
#include<pthread.h>
#include<semaphore.h>
#include<queue>
#include <unistd.h>
#include <random>

using namespace std;

int n;
int nUS;
int nPLUS;
int extra;
pthread_t threads[3];
vector<sem_t>sems;

void init_semaphore()
{
    sem_t temp;
    sem_init(&temp, 0, 2*n - 1);
    sems.push_back(temp);
    
    sem_t temp1;
    sem_init(&temp1, 0, 0);
    sems.push_back(temp1);
    
    sem_t temp2;
    sem_init(&temp2, 0, 0);
    sems.push_back(temp2);
   
}
void * us(void * arg)
{	

    char *msg = (char *) arg;
    while(1){
        // printf("us\n");
        sem_wait(&sems[0]);
        cout << '_';
        nUS++;
            // printf("%d %d\n", nUS, extra);

        if(nUS == extra){
            for(int i = 0; i < 2* n - extra; i++) sem_post(&sems[1]);
            if(extra == n) break;
        }
    }
}

void * pls(void * arg)
{	

    char *msg = (char *) arg;
    while(1){
        // printf("pls\n");

        sem_wait(&sems[1]);
        
        cout << '+';
        nPLUS++;
        if(nPLUS == 2 * n - extra) {
            sem_post(&sems[2]);
            if(extra == n) break;
        }
    }
}

void * nl(void * arg)
{	

    while(1){
        // printf("nl\n");
        sem_wait(&sems[2]);
        cout << endl;
        if(extra==n)break;
        extra--;
        nPLUS = 0;
        nUS = 0;
        
        for(int i = 0; i < extra; i++) sem_post(&sems[0]);
    }
}

int main(){
    cin >> n;
    extra = 2*n - 1;
    nUS = 0;
    nPLUS = 0;
    init_semaphore();
    pthread_create(&threads[0],NULL,us,(void*) "_");
    pthread_create(&threads[1],NULL,pls,(void*) "+");
    pthread_create(&threads[2],NULL,nl,(void*) "\n");


    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
    pthread_join(threads[2], NULL);
}