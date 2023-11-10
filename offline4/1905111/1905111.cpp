#include<bits/stdc++.h>
#include<iostream>
#include<pthread.h>
#include<semaphore.h>
#include<queue>
#include <unistd.h>
#include <random>

using namespace std;

// using poission distribution to generate random numbers (mean time : 8s)
std::default_random_engine generator;
std::poisson_distribution<int> distribution(8);

int nStudents, groupSize, nGroups, nPrinters, nBinders;
int printTime, bindTime, rwTime;
int nReaders = 0;
int nStuffs = 2;
int nSubmissions = 0;
vector<bool> hasArrived;
vector<bool> hasPrinted;
vector<bool> isPrinterFree;

queue<int> printers, binders;
// threads
pthread_t staffThreads[2];

// semaphores
sem_t binderSemaphore; // two binders concurrency
sem_t entrySemaphore;  // one entry book (read priority)
vector<sem_t>studentSemaphores; // one semaphore for each student to give them printers

// mutex
pthread_mutex_t printMutex; // for cout-ing
pthread_mutex_t entryMutex; // for handling entry book
pthread_mutex_t arriveMutex;// for handling arrival of students (and giving them printers)

time_t startTime, currTime;

int getTime(){
    time(&currTime);
    return (int) (currTime - startTime);
}

void init_semaphore()
{
	// mutex
    pthread_mutex_init(&printMutex, NULL);
    pthread_mutex_init(&entryMutex, NULL);
    pthread_mutex_init(&arriveMutex, NULL);

    // semaphores for binders
    sem_init(&entrySemaphore, 0, 1);
    sem_init(&binderSemaphore, 0, nBinders);

    for(int i = 0; i < nStudents; i++){
        sem_t temp;
        sem_init(&temp, 0, 0);
        studentSemaphores.push_back(temp);
    }
}

void writeToEntry(int groupNumber){
    sem_wait(&entrySemaphore);
    nSubmissions++;
    pthread_mutex_lock(&printMutex);
    cout << "Group " << groupNumber + 1 << " has started submission at time " << getTime() << endl;
    pthread_mutex_unlock(&printMutex);
    sleep(rwTime);

    pthread_mutex_lock(&printMutex);
    cout << "Group " << groupNumber + 1 << " has finished submission at time " << getTime() << endl;
    pthread_mutex_unlock(&printMutex);

    sem_post(&entrySemaphore);
}

void readEntryBook(int stuffNumber){
    pthread_mutex_lock(&entryMutex);
    nReaders++;
    if(nReaders == 1){
        sem_wait(&entrySemaphore);
    }
    pthread_mutex_lock(&printMutex);
    cout << "Stuff " << stuffNumber + 1 << " has started reading the entry book at time " << getTime() 
    << ". No. of submissions = " << nSubmissions << endl;
    pthread_mutex_unlock(&printMutex);
    pthread_mutex_unlock(&entryMutex);

    sleep(rwTime);

    pthread_mutex_lock(&entryMutex);
    nReaders--;
    if(nReaders == 0){
        sem_post(&entrySemaphore);
    }
    pthread_mutex_lock(&printMutex);
    cout << "Stuff " << stuffNumber + 1 << " has finished reading the entry book at time " << getTime() 
    << ". No. of submissions = " << nSubmissions << endl;
    pthread_mutex_unlock(&printMutex);
    pthread_mutex_unlock(&entryMutex);
}

// stuff work
void * stuffWork(void * arg)
{	
    // all the stuff work goes here
    // getting stuff number
    int *stuffNumber = (int *) arg;
    int timeInterval = 5 + rand() % 5;
    while(1){
        sleep(timeInterval);
        readEntryBook(*stuffNumber);
    }
}

// use printer
void usePrinter(int studentNumber){
    int printNumber = studentNumber % nPrinters;

    // test if possible
    pthread_mutex_lock(&arriveMutex);

    pthread_mutex_lock(&printMutex);
    cout << "Student " << studentNumber + 1 << " has arrived at the print station at time " << getTime() << endl;
    pthread_mutex_unlock(&printMutex);

    hasArrived[studentNumber] = true;
    if(isPrinterFree[printNumber] && hasArrived[studentNumber]) {
        sem_post(&studentSemaphores[studentNumber]);
        isPrinterFree[printNumber] = false;
    }
    pthread_mutex_unlock(&arriveMutex);

    sem_wait(&studentSemaphores[studentNumber]);

    pthread_mutex_lock(&printMutex);
    cout << "Student " << studentNumber + 1 << " has started printing using Printer " << printNumber + 1 << " at time " << getTime() << endl;
    pthread_mutex_unlock(&printMutex);

    sleep(printTime);

    pthread_mutex_lock(&arriveMutex);
    pthread_mutex_lock(&printMutex);
    cout << "Student " << studentNumber + 1 << " has finished printing at time " << getTime() << endl;
    pthread_mutex_unlock(&printMutex);

    isPrinterFree[printNumber] = true;
    hasPrinted[studentNumber] = true;

    int groupNumber = studentNumber / groupSize;

    // first try group members
    for(int i = 0; i < groupSize; i++){
        int tempStudentNumber = groupNumber * groupSize + i;
        if(hasPrinted[tempStudentNumber]) continue; // you are already done!
        if(tempStudentNumber % nPrinters != printNumber) continue; // ignore if not same printer

        if(isPrinterFree[printNumber] & hasArrived[tempStudentNumber]) {
            cout << studentNumber + 1 << " is giving printer to group member " << tempStudentNumber + 1 << endl;
            sem_post(&studentSemaphores[tempStudentNumber]);
            pthread_mutex_unlock(&arriveMutex);
            return;
        }
    }
    // now try non group members
    for(int i = 0; i < nStudents; i++){
        int tempStudentNumber = i;
        if(hasPrinted[tempStudentNumber]) continue; // you are already done!
        if(tempStudentNumber % nPrinters != printNumber || tempStudentNumber / nGroups == groupNumber) continue; // ignore if not same printer or same group

        if(isPrinterFree[printNumber] & hasArrived[tempStudentNumber]) {
            cout << studentNumber + 1 << " is giving printer to non-group member " << tempStudentNumber + 1 << endl;
            sem_post(&studentSemaphores[tempStudentNumber]);
            pthread_mutex_unlock(&arriveMutex);
            return;
        }
    }
    pthread_mutex_unlock(&arriveMutex);

}

// use binder
void useBinder(int groupNumber){
    sem_wait(&binderSemaphore);

    pthread_mutex_lock(&printMutex);
    cout << "Group " << groupNumber + 1 << " has started binding at time " << getTime() << endl;
    pthread_mutex_unlock(&printMutex);
    
    sleep(bindTime);

    pthread_mutex_lock(&printMutex);
    cout << "Group " << groupNumber + 1 << " has finished binding at time " << getTime() << endl;
    pthread_mutex_unlock(&printMutex);

    sem_post(&binderSemaphore);
}

// print work
void * PrintWork(void * arg)
{	
    // all the print work goes here
    // getting student number
    int *studentNumber = (int *) arg;
    int randomDelay = distribution(generator);
    sleep(randomDelay);


    usePrinter(*studentNumber);
}

// group work
void * GroupWork(void * arg)
{	
    // all the group work goes here
    // getting group number
    int *groupNum = (int *) arg;

    // cout << "Group " << *groupNum << " is formed at " << getTime() << endl;
    
    // init student threads
    vector<pthread_t> studentThreads(groupSize);

    // create student threads
    for(int i = 0; i < groupSize; i++){
        int *studentNumber = new int;
        *studentNumber = *groupNum * groupSize + i;
        pthread_create(&studentThreads[i],NULL,PrintWork,(void*) studentNumber);
    }

    // sync student threads
    for(int i = 0; i < groupSize; i++){
        pthread_join(studentThreads[i],NULL);
    }

    // leader does binding
    useBinder(*groupNum);

    // leader does submission
    writeToEntry(*groupNum);
    // done
}

int main(void){
    // take input from input.txt
    freopen("input.txt","r",stdin);

    // take input
    cin >> nStudents >> groupSize;
    
    nPrinters = 4;
    nBinders = 2;
    nGroups = nStudents / groupSize;
    
    cin >> printTime >> bindTime >> rwTime;

    

    // init resources (4 printers & students) 
    hasArrived.resize(nStudents);
    hasPrinted.resize(nStudents);
    isPrinterFree.resize(nPrinters, true);

    // init mutex and semaphore
    init_semaphore();
    // cout << "Mutex and semaphores initialized" << endl;

    // init sim time
    time(&startTime);

    // staff thread creation
    for(int i = 0; i < nStuffs; i++){
        int *stuffNumber = new int;
        *stuffNumber = i;
        pthread_create(&staffThreads[i],NULL,stuffWork,(void*) stuffNumber);
    }
    // cout << "Staff threads created" << endl;

    // group thread creation
    vector<pthread_t> groupThreads(nGroups);
    for(int i = 0; i < nGroups; i++){
        // int to char *
        
        int *groupNumber = new int;
        *groupNumber = i;
        
        pthread_create(&groupThreads[i],NULL,GroupWork,(void*) groupNumber);
    }
    // cout << "Group threads created" << endl;

    // syncing group threads
    for(int i = 0; i < nGroups; i++)
	{
		pthread_join(groupThreads[i],NULL);
	}
    // pthread_join(staffThreads[0],NULL);
    // pthread_join(staffThreads[1],NULL);
}