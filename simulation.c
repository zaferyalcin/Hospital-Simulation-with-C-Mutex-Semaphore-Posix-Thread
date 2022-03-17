#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

// The number of availabe places.
int REGISTRATION_SIZE = 10;
int RESTROOM_SIZE = 10;
int CAFE_NUMBER = 10;
int GP_NUMBER = 10;
int PHARMACY_NUMBER = 10;
int BLOOD_LAB_NUMBER = 10;

// The number of operating rooms, surgeons, and nurses that are available
int OR_NUMBER = 10;
int SURGEON_NUMBER = 30;
int NURSE_NUMBER = 30;

// The maximum number of surgeons and nurses that can do a surgery. (Filling with random value that between 1 and 5)
int SURGEON_LIMIT = 5;
int NURSE_LIMIT = 5;

// Patient number that will be generated.
int PATIENT_NUMBER = 1000;

// TOTAL GAIN FOR HOSPITAL
int HOSPITAL_WALLET = 0;

// TIME CALCULATION VALUES (Filling with random value that between 1 and given values)
// Given values in milliseconds*
int WAIT_TIME = 100;
int REGISTRATION_TIME = 100;
int GP_TIME = 200;
int PHARMACY_TIME = 100;
int BLOOD_LAB_TIME = 200;
int SURGERY_TIME = 500;
int CAFE_TIME = 100;
int RESTROOM_TIME = 100;

// COST CALCULATION VALUES
int REGISTRATION_COST = 100;
int PHARMACY_COST = 200; // Should be calculated randomly between 1- and given
int BLOOD_LAB_COST = 200;
int SURGERY_OR_COST = 200;
int SURGERY_SURGEON_COST = 100;
int SURGERY_NURSE_COST = 50;
int CAFE_COST = 200; // Should be calculated randomly between 1- and given

// HUNGER AND RESTROOM RATES
int HUNGER_INCREASE_RATE = 10;
int RESTROOM_INCREASE_RATE = 10;

// SEMAPHORES 
sem_t registration_sem;
sem_t cafe_sem;
sem_t restroom_sem;
sem_t gp_office_sem;
sem_t blood_lab_sem;
sem_t pharmacy_sem;
sem_t nurse_sem;
sem_t surgeon_sem;
sem_t OR_sem;

// MUTEXES
pthread_mutex_t wallet;
pthread_mutex_t nurse_mutex;
pthread_mutex_t surgeon_mutex;

// DEFIND FUNCTIONS WITH PARAMETER
void rand_wait(int milis); // Sleeps in random value between 1 and entered number (in ms)
int rand_value(int val); // Returns a random value between 1 and entered number.
void cafe(int patient_ID); // Cafe operations
void restroom(int patient_ID); // Restroom operations
void *patient(void* args); // Creates the instance of the patient and takes patient to the registration room
void gp_office(int patient_ID); // GP office operations.
void pharmacy(int patient_ID, int *hunger_meter, int *restroom_meter); // Pharmacy operations
void blood_lab(int patient_ID, int *hunger_meter, int *restroom_meter); // Blood Lab operations
void surgery(int patient_ID, int *hunger_meter, int *restroom_meter); // Surgery operations
void check_cafe_and_restroom(int patient_ID, int *hunger_meter, int *restroom_meter); // Checks and increase hunger and need for restroom

// FUNCTIONS

int main(int argc, char *argv) {
    // defining patient possix thread arrays with given patient number
    pthread_t patient_t[PATIENT_NUMBER];

    // initialising semaphores
    sem_init(&registration_sem, 0, REGISTRATION_SIZE);
    sem_init(&gp_office_sem, 0, GP_NUMBER);
    sem_init(&blood_lab_sem, 0, BLOOD_LAB_NUMBER);
    sem_init(&pharmacy_sem, 0, PHARMACY_NUMBER);
    sem_init(&nurse_sem, 0, NURSE_NUMBER);
    sem_init(&surgeon_sem, 0, SURGEON_NUMBER);
    sem_init(&OR_sem, 0, OR_NUMBER);
    sem_init(&cafe_sem, 0, CAFE_NUMBER);
    sem_init(&restroom_sem, 0, RESTROOM_SIZE);

    // creating every thread with *patient function
    for (int i = 0; i < PATIENT_NUMBER; i++) {
        int *a = malloc(sizeof(int)); // for passing patient_ID
        *a = i;
        if (pthread_create(&patient_t[i], NULL, &patient, a) != 0)
            perror("Failed to create patient.");

    }
    for (int i = 0; i < PATIENT_NUMBER; i++) {
        if (pthread_join(patient_t[i], NULL) != 0)
            perror("Failed to join patient.");
    }

    printf("\n\nHospital Wallet : %d\n\n",HOSPITAL_WALLET);
    
    // destroying every semephore that initialize.
    sem_destroy(&registration_sem);
    sem_destroy(&gp_office_sem);
    sem_destroy(&blood_lab_sem);
    sem_destroy(&pharmacy_sem);
    sem_destroy(&nurse_sem);
    sem_destroy(&surgeon_sem);
    sem_destroy(&OR_sem);
    sem_destroy(&cafe_sem);
    sem_destroy(&restroom_sem);

    return 0;
}

void *patient(void *args) {

    int patient_ID = *(int *) args; // takes Patient_ID as a pointer.
    int patient_disease = rand() % 3; // 0 represents medicine, 1 represents surgery, 2 represents blood lab
    int patient_hunger_meter = rand_value(100); // randomly assigns hunger meter between 1-100
    int patient_restroom_meter = rand_value(100); // randomly assigns restroom meter between 1-100

    printf("Patient %d has arrived to hospital and waiting for registration.\n", patient_ID);
    
    // sleeps in given time
    rand_wait(WAIT_TIME);
    check_cafe_and_restroom(patient_ID, &patient_hunger_meter, &patient_restroom_meter);
    
    // locks semaphore 
    sem_wait(&registration_sem);

    printf("Patient %d now in registration\n", patient_ID);

    // sleeps in given time
    rand_wait(REGISTRATION_TIME);
    
    // locks the wallet for the thread so that concurrent threads to keep the value intact.
    pthread_mutex_lock(&wallet); 

    HOSPITAL_WALLET += REGISTRATION_COST;

    // unlocking mutex.
    pthread_mutex_unlock(&wallet);

    printf("Patient %d payed %d for registration.\n\tHOSPITAL_WALLET increase to %d\n\n", patient_ID, REGISTRATION_COST,
           HOSPITAL_WALLET);
    printf("Patient %d left the registration room.\n\n", patient_ID);

    // unlock semaphore
    sem_post(&registration_sem);
    check_cafe_and_restroom(patient_ID, &patient_hunger_meter, &patient_restroom_meter);

    // calling gp_office
    gp_office(patient_ID);
    check_cafe_and_restroom(patient_ID, &patient_hunger_meter, &patient_restroom_meter);
    
    // after gp_office, call the function; pharmacy, surgery, blood lab according to the patient's need.
    if (patient_disease == 0) {
        pharmacy(patient_ID, &patient_hunger_meter, &patient_restroom_meter);
    } else if (patient_disease == 1) {
        surgery(patient_ID, &patient_hunger_meter, &patient_restroom_meter);
    } else if (patient_disease == 2) {
        blood_lab(patient_ID, &patient_hunger_meter, &patient_restroom_meter); 
    }


    free(args);
}

void gp_office(int patient_ID) {

    printf("Patient %d waiting for GP office.\n", patient_ID);

    // sleeps in given time
    rand_wait(WAIT_TIME);

    //lock sempahore
    sem_wait(&gp_office_sem);

    printf("Patient %d now in GP office\n", patient_ID);

    // sleeps in given time
    rand_wait(GP_TIME);

    printf("Patient %d left the GP office.\n\n", patient_ID);

    // unlock sempahore
    sem_post(&gp_office_sem);
}

void blood_lab(int patient_ID, int *hunger_meter, int *restroom_meter) {

    printf("Patient %d waiting for blood lab.\n", patient_ID);

    // sleeps in given time
    rand_wait(WAIT_TIME);
    check_cafe_and_restroom(patient_ID, hunger_meter, restroom_meter);

    sem_wait(&blood_lab_sem);

    printf("Patient %d now in blood lab\n", patient_ID);

    // sleeps in given time
    rand_wait(BLOOD_LAB_TIME);

    pthread_mutex_lock(&wallet);

    // increase hospital value with constant
    HOSPITAL_WALLET += BLOOD_LAB_COST; 

    printf("Patient %d payed %d for blood test.\n\tHOSPITAL_WALLET increase to %d\n\n", patient_ID, REGISTRATION_COST,
           HOSPITAL_WALLET);

    pthread_mutex_unlock(&wallet);

    printf("Patient %d left the blood lab.\n\n", patient_ID);

    sem_post(&blood_lab_sem);
    check_cafe_and_restroom(patient_ID, hunger_meter, restroom_meter);

    // after the blood test patients goes gp_office
    gp_office(patient_ID);
    check_cafe_and_restroom(patient_ID, hunger_meter, restroom_meter);
    
    // Randomly determines whether medicine is required for patient who has had a blood test
    int medicine = (int) (rand() % 2);

    // if medicine equals to 1 that means patient needs medicine
    if (medicine == 1) pharmacy(patient_ID, hunger_meter, restroom_meter);
}

void surgery(int patient_ID, int *hunger_meter, int *restroom_meter) {
    
    // randomly creates nurse and surgeon 
    int nurse = rand() % NURSE_LIMIT + 1;
    int surgeon = rand() % SURGEON_LIMIT + 1;

    printf("Patient %d waiting for surgery.\n", patient_ID);

    // sleeps in given time
    rand_wait(WAIT_TIME);
    check_cafe_and_restroom(patient_ID, hunger_meter, restroom_meter);

    // locks the nurse mutex .
    pthread_mutex_lock(&nurse_mutex);

    // locks all sempahores for nurse
    for (int i = 0; i < nurse; ++i) {
        sem_wait(&nurse_sem);
    }

    // unlock nurse mutex.
    pthread_mutex_unlock(&nurse_mutex);

    // lock surgeon mutex
    pthread_mutex_lock(&surgeon_mutex);

    for (int i = 0; i < surgeon; ++i) {
        sem_wait(&surgeon_sem);
    }

    // unlock surgeon mutex 
    pthread_mutex_unlock(&surgeon_mutex);

    sem_wait(&OR_sem);

    pthread_mutex_lock(&wallet);

    // calculating surgery_cost with given formula.
    int surgery_cost = SURGERY_OR_COST+SURGERY_NURSE_COST*nurse+surgeon*SURGERY_SURGEON_COST;
    HOSPITAL_WALLET += surgery_cost;

    printf("Patient %d payed %d for surgery.\n\tHOSPITAL_WALLET increase to %d\n\n", patient_ID, surgery_cost,
           HOSPITAL_WALLET);

    pthread_mutex_unlock(&wallet);

    printf("Patient %d now in surgery\n", patient_ID);

    // sleeps in given time
    rand_wait(SURGERY_TIME);

    printf("Patient %d left the operation room.\n\n", patient_ID);

    sem_post(&OR_sem);

    // after operations unlocks the nurse and surgeon semaphores.
    for (int i = 0; i < nurse; ++i) {
        sem_post(&nurse_sem);
    }
    for (int i = 0; i < surgeon; ++i) {
        sem_post(&surgeon_sem);
    }
}

void pharmacy(int patient_ID, int *hunger_meter, int *restroom_meter) {
    printf("Patient %d waiting for pharmacy.\n", patient_ID);

    // sleeps in given time
    rand_wait(WAIT_TIME);
    check_cafe_and_restroom(patient_ID, hunger_meter, restroom_meter);

    sem_wait(&pharmacy_sem);

    printf("Patient %d now in pharmacy\n", patient_ID);

    // sleeps in given time
    rand_wait(PHARMACY_TIME);

    pthread_mutex_lock(&wallet);

    // calculates random cost for pharmacy
    int pharmacy_cost = rand_value(PHARMACY_COST);

    HOSPITAL_WALLET += pharmacy_cost;

    printf("Patient %d payed %d for medicine.\n\tHOSPITAL_WALLET increase to %d\n\n", patient_ID, pharmacy_cost,
           HOSPITAL_WALLET);

    pthread_mutex_unlock(&wallet);

    printf("Patient %d left the pharmacy.\n\n", patient_ID);

    sem_post(&pharmacy_sem);
    check_cafe_and_restroom(patient_ID, hunger_meter, restroom_meter);
}

void restroom(int patient_ID){
    printf("Patient %d waiting for Restroom.\n", patient_ID);

    // sleeps in given time
    rand_wait(WAIT_TIME);

    sem_wait(&restroom_sem);

    printf("Patient %d now in Restroom\n", patient_ID);

    // sleeps in given time
    rand_wait(RESTROOM_TIME);

    printf("Patient %d left the restroom.\n\n", patient_ID);

    sem_post(&restroom_sem);
}

void cafe(int patient_ID){
    printf("Patient %d waiting for Cafe.\n", patient_ID);

    // sleeps in given time
    rand_wait(WAIT_TIME);

    sem_wait(&cafe_sem);
 
    printf("Patient %d now in cafe\n", patient_ID);

    // sleeps in given time
    rand_wait(CAFE_TIME);

    pthread_mutex_lock(&wallet);

    // calculates random cost for cafe
    int cafe_cost = rand_value(CAFE_COST);

    HOSPITAL_WALLET += cafe_cost;

    printf("Patient %d payed %d for cafe.\n\tHOSPITAL_WALLET increase to %d\n\n", patient_ID, cafe_cost,
           HOSPITAL_WALLET);

    pthread_mutex_unlock(&wallet);

    printf("Patient %d left the cafe.\n\n", patient_ID);

    sem_post(&cafe_sem);
}

void check_cafe_and_restroom(int patient_ID, int *hunger_meter, int *restroom_meter){ 
    // if hunger meter over 100 then calls the cafe function. 
    if (*hunger_meter >= 100)
    {
        (*hunger_meter) = 0;
        cafe(patient_ID);
       
    }else{ // if it's not then increase the hunger meter value with given increase rate.
        (*hunger_meter) += rand_value(HUNGER_INCREASE_RATE);
    }

    // if restroom meter over 100 then calls the restroom function.
    if (*restroom_meter >= 100)
    {
        (*restroom_meter) = 0;
        restroom(patient_ID);
    }else{ // if it's not then increase the restroom meter value with given increase rate.
        (*restroom_meter) += rand_value(RESTROOM_INCREASE_RATE); 
    }
}

int rand_value(int val){
    return rand() %val + 1;
}

void rand_wait(int milis) { 
    double len;
    len = (double) (rand() % milis + 1) / 1000;
    sleep(len);
}

