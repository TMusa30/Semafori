#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/shm.h>
#include <time.h>
#include <semaphore.h>
#include <pthread.h>



int BUD, BRD, BID;

char **UMS;
char **IMS;

sem_t **UMS_sem;
sem_t **IMS_sem;

sem_t **UMS_podaci_sem;

int **ULAZ;
int **IZLAZ;

int velicina;



char dohvati_ulaz(int I){
    return 'A' + rand() % 26;
}

char obradi_ulaz(int I, char U){
    int wait_time = (rand() % 1)+1;
    sleep(wait_time);
    return rand() % BRD;
}

void obradi(int J, char P, char *r, int *t){
    int wait_time = (rand() % 2) + 2;
    sleep(wait_time);
    *r = tolower(P);
    *t = rand() % BID;
}

void prikaz_meduspremnika(){
    printf("UMS[] : ");
    for(int i=0;i<BRD;i++){
        for(int j=0;j<velicina;j++){
            printf("%c", UMS[i][j]);
        }
        printf("\t");
    }

    printf("\nIMS[] : ");

    for(int i=0;i<BID;i++){
        for(int j=0;j<velicina;j++){
            printf("%c", IMS[i][j]);
        }
        printf("\t");
    }
    printf("\n");
}

void kruzno_povecaj(int* broj){
    *broj = (*broj + 1)%velicina;
}

void *ulazna(void *thread_id) {
    int I = *((int *)thread_id);
    while(1){
        int wait_time = (rand() % 6) + 5;
        sleep(wait_time);

        char U = dohvati_ulaz(I);
        int T = obradi_ulaz(I, U);

        sem_wait(UMS_sem[T]);

        if(UMS[T][ULAZ[0][T]]!='-'){
            kruzno_povecaj(&IZLAZ[0][T]);
        }
        UMS[T][ULAZ[0][T]]=U;

        kruzno_povecaj(&ULAZ[0][T]);

        printf("U%d: dohvati_ulaz(%d) => %c; obradi_ulaz(%c) => %d; %c => UMS[%d]\n", I, I, U, U, T, U, T);
        prikaz_meduspremnika();

        sem_post(UMS_podaci_sem[T]);
        sem_post(UMS_sem[T]);

    }
}

void *radna(void *thread_id) {
    int I = *((int *)thread_id);
    while(1){
        sem_wait(UMS_podaci_sem[I]);
        
        sem_wait(UMS_sem[I]);

        char P, r;
        int t;
        P = UMS[I][IZLAZ[0][I]];
        UMS[I][IZLAZ[0][I]]='-';
        kruzno_povecaj(&IZLAZ[0][I]);
        printf("R%d: UMS[%d] => %c\n", I, I, P);
        prikaz_meduspremnika();
        sem_post(UMS_sem[I]);


        obradi(I, P, &r, &t);

        
        sem_wait(IMS_sem[t]);

        if(IMS[t][ULAZ[1][t]]!='-'){
            kruzno_povecaj(&IZLAZ[1][t]);
        }
        IMS[t][ULAZ[1][t]]=r;
        kruzno_povecaj(&ULAZ[1][t]);

        printf("R%d: podaci su obrađeni i %c => IMS[%d]\n", I, r, t);
        prikaz_meduspremnika();

        sem_post(IMS_sem[t]);

    }
}

void *izlazna(void *thread_id) {
    int I = *((int *)thread_id);
    char za_ispis = '0';
    while(1){
        sleep(5);

        
        sem_wait(IMS_sem[I]);

        if(IMS[I][IZLAZ[1][I]]!='-'){
            za_ispis=IMS[I][IZLAZ[1][I]];
            IMS[I][IZLAZ[1][I]]='-';
            kruzno_povecaj(&IZLAZ[1][I]);
        }

        printf("I%d: printam %c\n", I, za_ispis);
        prikaz_meduspremnika();

        sem_post(IMS_sem[I]);

        
    }
}

int main(int argc, char **argv){

    srand(time(NULL));

    BUD=atoi(argv[1]);
    BRD=atoi(argv[2]);
    BID=atoi(argv[3]);
    velicina=atoi(argv[4]);

    UMS = (char **)malloc(BRD*sizeof(char *));
    IMS = (char **)malloc(BID*sizeof(char *));

    ULAZ = (int **)malloc(2*sizeof(int *));
    IZLAZ = (int **)malloc(2*sizeof(int *));

    ULAZ[0] = (int *)malloc(BRD*sizeof(int));
    IZLAZ[0] = (int *)malloc(BRD*sizeof(int));
    ULAZ[1] = (int *)malloc(BID*sizeof(int));
    IZLAZ[1] = (int *)malloc(BID*sizeof(int));


    for (int i = 0; i < BRD; i++) {
        UMS[i] = (char *)malloc(velicina * sizeof(char));
    }

    for (int i = 0; i < BID; i++) {
        IMS[i] = (char *)malloc(velicina * sizeof(char));
    }

    for(int i=0;i<BRD;i++){
        for(int j=0;j<velicina;j++){
            UMS[i][j]='-';
        }
    }

    for(int i=0;i<BID;i++){
        for(int j=0;j<velicina;j++){
            IMS[i][j]='-';
        }
    }

    for (int i = 0; i < BRD; i++) {
        ULAZ[0][i] = 0;
        IZLAZ[0][i] = 0;
    }

    for (int i = 0; i < BID; i++) {
        ULAZ[1][i] = 0;
        IZLAZ[1][i] = 0;
    }
    

    UMS_sem = (sem_t **)malloc(BRD*sizeof(sem_t *));
    IMS_sem = (sem_t **)malloc(BID*sizeof(sem_t *));
    UMS_podaci_sem = (sem_t **)malloc(BRD*sizeof(sem_t *));


    for (int i = 0; i < BRD; i++) {
        UMS_sem[i] = (sem_t *)malloc(sizeof(sem_t));
        sem_init(UMS_sem[i], 0, 1);
    }

    for (int i = 0; i < BID; i++) {
        IMS_sem[i] = (sem_t *)malloc(sizeof(sem_t));
        sem_init(IMS_sem[i], 0, 1);
    }

    for (int i = 0; i < BRD; i++) {
        UMS_podaci_sem[i] = (sem_t *)malloc(sizeof(sem_t));
        sem_init(UMS_podaci_sem[i], 0, 0);
    }


    pthread_t U_threads[BUD];
    int U_threads_ids[BUD];
    pthread_t R_threads[BRD];
    int R_threads_ids[BRD];
    pthread_t I_threads[BID];
    int I_threads_ids[BID];

    prikaz_meduspremnika();

    for (int i=0; i < BUD; i++) {
        U_threads_ids[i]=i;
        pthread_create(&U_threads[i], NULL, ulazna, (void *)&U_threads_ids[i]);
    }

    sleep(25);

    for (int i=0; i < BRD; i++) {
        R_threads_ids[i]=i;
        pthread_create(&R_threads[i], NULL, radna, (void *)&R_threads_ids[i]);
    }

    for (int i=0; i < BID; i++) {
        I_threads_ids[i]=i;
        pthread_create(&I_threads[i], NULL, izlazna, (void *)&I_threads_ids[i]);
    }


    for (int i = 0; i < BUD; i++) {
        pthread_join(U_threads[i], NULL);
    }
    for (int i = 0; i < BRD; i++) {
        pthread_join(R_threads[i], NULL);
    }
    for (int i = 0; i < BID; i++) {
        pthread_join(I_threads[i], NULL);
    }
}