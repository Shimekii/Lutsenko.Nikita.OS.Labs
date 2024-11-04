#include <iostream>
#include <pthread.h>
#include <unistd.h> 

using namespace std;

pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int ready = 0; 

void* providerThread(void*) {
    while (true) {
        pthread_mutex_lock(&lock);
        
        if (ready == 1) {
            pthread_mutex_unlock(&lock);
            continue;
        }

        ready = 1;
        cout << "Поставщик: событие отправлено." << endl;
        
        pthread_cond_signal(&cond1);
        
        pthread_mutex_unlock(&lock);
        
        sleep(1);
    }
    return nullptr;
}

void* consumerThread(void*) {
    while (true) {
        pthread_mutex_lock(&lock);
        
        while (ready == 0) {
            pthread_cond_wait(&cond1, &lock);
            cout << "Потребитель: событие получено." << endl;
        }

        ready = 0;
        cout << "Потребитель: событие обработано." << endl;
        
        pthread_mutex_unlock(&lock);
    }
    return nullptr;
}

int main() {
    pthread_t provider, consumer;
    
    pthread_create(&provider, nullptr, providerThread, nullptr);
    pthread_create(&consumer, nullptr, consumerThread, nullptr);
    
    pthread_join(provider, nullptr);
    pthread_join(consumer, nullptr);
    
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond1);
    
    return 0;
}
