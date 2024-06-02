/*
    Γεώργιος Τσοράκος - 3220217
    Φίλιππος Τοτόμης  - 3220203
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include "3220217-3220203.h"

int available_tel = N_TEL;
int available_cook = N_COOK;
int available_oven = N_OVEN;
int available_delivery = N_DELIVERY;
unsigned int seed;
int total = 0, remainingOrders;
int failedPayments = 0, successfullPayments = 0;
int type_sales[3] = { 0 };
struct timespec max_serv_time, avg_serv_time;
struct timespec max_cold_time, avg_cold_time;

pthread_mutex_t mutexOut, mutexTel, mutexCook, mutexOven, mutexDel;
pthread_cond_t condFindTel, condFindCook, condFindOven, condFindDelivery;

//NOTE: STEPS
// 1) Τηλεφωνητής (tel). Αν δεν υπάρχει τηλεφωνητής διαθέσιμος τότε ο πελάτης περιμένει.
// 2) Επιλέγουμε τις πίτσες και περιμένουμε Τ τυχαία δευτερόλεπτα και καταχωρούμε τα έσοδα.
// 3) Περιμένουμε μέχρι κάποιος παρασκευαστής (cook) να γίνει διαθέσιμος.
// 4) Όταν γίνει διαθέσιμος, περιμένουμε Τ_prep χρόνο για να ετοιμαστεί η κάθε πίτσα.
// 5) Περιμένουμε μέχρι να γίνουν αρκετοί φούρνοι (oven) διαθέσιμοι και όταν γίνουν οι πίτσες μπαίνουν στους φούρνους και ο παρασκευαστής αναλαμβάνει άλλη παραγγελία.
// Οι πίτσες ψήνονται για χρόνο Τ_bake.
// 6) Περιμένουμε μέχρι κάποιος διανομέας (delivery) να γίνει διαθέσιμος.
// 7) Περιμένουμε Τ_pack χρόνο για να πακεταριστούν οι πίτσες.
// 8) Περιμένουμε 2*[Τ_dellow, T_delhigh] χρόνο για να ξαναγίνει διαθέσιμος ο διανομέας και να τελειώσει η συνάρτηση.

int main(int argc, char **argv) {

    if (argc != 3) {
        fprintf(stderr, "Invalid argument count. Expected 2 arguments, got %d.\n", argc - 1);
        return ARGUMENT_ERROR;
    }

    int totalCustomers;
    if ((totalCustomers = atoi(argv[1])) == 0) {
        fprintf(stderr, "Invalid argument for total customers: %s\n", argv[1]);
        return ARGUMENT_ERROR;
    }
    remainingOrders = totalCustomers;

    if ((seed = atoi(argv[2])) == 0) {
        fprintf(stderr, "Invalid argument for seed: %s\n", argv[2]);
        return ARGUMENT_ERROR;
    }
    // srand(seed);

    if (pthread_mutex_init(&mutexOut, NULL) != 0 ||
        pthread_mutex_init(&mutexTel, NULL) != 0 ||
        pthread_mutex_init(&mutexCook, NULL) != 0 ||
        pthread_mutex_init(&mutexOven, NULL) != 0 ||
        pthread_mutex_init(&mutexDel, NULL) != 0) {
        fprintf(stderr, "Failed to initialize mutexes.\n");
        return MUTEX_INIT_ERROR;
    }

    if (pthread_cond_init(&condFindTel, NULL) != 0 ||
        pthread_cond_init(&condFindCook, NULL) != 0 ||
        pthread_cond_init(&condFindOven, NULL) != 0 ||
        pthread_cond_init(&condFindDelivery, NULL) != 0) {
        fprintf(stderr, "Failed to initialize condition variables.\n");
        return COND_INIT_ERROR;
    }

    pthread_t threads[totalCustomers];
    // void **threadIDS = malloc(totalCustomers * sizeof(void *));
    // if (threadIDS == NULL) {
    //     fprintf(stderr, "Failed to allocate memory for thread IDS.\n");
    //     clean_up(totalCustomers, threadIDS, MEMORY_ALLOC_ERROR);
    // }
    for (int i = 0; i < totalCustomers; ++i) {
        thread_args_t* threadArgs = (thread_args_t*)malloc(sizeof(thread_args_t));
        // threadIDS[i] = malloc(sizeof(thread_args_t));
        if (threadArgs == NULL) {
            fprintf(stderr, "Failed to allocate memory for thread_args.\n");
            clean_up(totalCustomers, MEMORY_ALLOC_ERROR);
        }
        threadArgs->oid = i;
        threadArgs->seed = &seed;
        if (pthread_create(&threads[i], NULL, order, (void*)threadArgs) != 0) {
            fprintf(stderr, "Failed to create thread (%d).\n", i);
            free(threadArgs);
            clean_up(totalCustomers, THREAD_CREATE_ERROR);
        }
    }

    for (int i = 0; i < totalCustomers; ++i) {
        if (pthread_join(threads[i], NULL) != 0) {
            fprintf(stderr, "Failed to join thread (%d).\n", i);
            clean_up(totalCustomers, THREAD_JOIN_ERROR);
        }
    }

    printf("%-27s %d\n", "Margarita pizzas ordered: ", type_sales[PIZZA_MARGARITA]);
    printf("%-27s %d\n", "Peperoni pizzas ordered: ", type_sales[PIZZA_PEPERONI]);
    printf("%-27s %d\n", "Special pizzas ordered: ", type_sales[PIZZA_SPECIAL]);

    printf("%-27s %d\n", "Successful payments:", successfullPayments);
    printf("%-27s %d\n", "Failed payments:", failedPayments);

    printf("%-27s %ld minutes\n", "Max service time:", timespec_to_minutes(max_serv_time));
    printf("%-27s %.2f minutes\n", "Average service time:", ((float)timespec_to_minutes(avg_serv_time)) / successfullPayments);

    printf("%-27s %ld minutes\n", "Max cold time:", timespec_to_minutes(max_cold_time));
    printf("%-27s %.2f minutes\n", "Average cold time:", ((float)timespec_to_minutes(avg_cold_time)) / successfullPayments);
    printf("%-27s %d\n", "Total:", total);


    clean_up(totalCustomers, SUCCESS);
}

int bounded_rand(int lower_bound, int upper_bound, unsigned int *seed) {
    return lower_bound + rand_r(seed) % (upper_bound - lower_bound + 1);
}

void clean_up(int totalCustomers, int errorCode) {
    pthread_cond_destroy(&condFindTel);
    pthread_cond_destroy(&condFindCook);
    pthread_cond_destroy(&condFindOven);
    pthread_cond_destroy(&condFindDelivery);

    pthread_mutex_destroy(&mutexOut);
    pthread_mutex_destroy(&mutexTel);
    pthread_mutex_destroy(&mutexCook);
    pthread_mutex_destroy(&mutexOven);
    pthread_mutex_destroy(&mutexDel);

    exit(errorCode);
}

static inline void getTime_r(struct timespec* __time, struct tm* timeInfo, char* buffer){
    clock_gettime(CLOCK_REALTIME, __time);
    localtime_r(&__time->tv_sec, timeInfo);
    asctime_r(timeInfo, buffer);
    buffer[24] = '\0';
}

static inline struct timespec getTimeDiff(struct timespec* start, struct timespec* end){
    struct timespec diff;
    if ((end->tv_nsec - start->tv_nsec) < 0) {
        diff.tv_sec = end->tv_sec - start->tv_sec - 1;
        diff.tv_nsec = 1000000000 + end->tv_nsec - start->tv_nsec;
    } else {
        diff.tv_sec = end->tv_sec - start->tv_sec;
        diff.tv_nsec = end->tv_nsec - start->tv_nsec;
    }
    return diff;
}

int compare_timespec(struct timespec* t1, struct timespec* t2) {
    if (t1->tv_sec < t2->tv_sec)
        return -1;
    else if (t1->tv_sec > t2->tv_sec)
        return 1;
    else {
        if (t1->tv_nsec < t2->tv_nsec) 
            return -1;
        else if (t1->tv_nsec > t2->tv_nsec)
            return 1;
        else
            return 0;
    }
}

static inline time_t timespec_to_minutes(struct timespec ts) {
    return (ts.tv_sec + ts.tv_nsec / NANOSECONDS_PER_SECOND);
}

struct timespec add_timespecs(struct timespec t1, struct timespec t2) {
    struct timespec result;
    
    result.tv_sec = t1.tv_sec + t2.tv_sec;
    result.tv_nsec = t1.tv_nsec + t2.tv_nsec;

    if (result.tv_nsec >= NANOSECONDS_PER_SECOND) {
        result.tv_sec += 1;
        result.tv_nsec -= NANOSECONDS_PER_SECOND;
    }

    return result;
}

void *order(void *__threadArgs) {
    char time_buffer[26];
    struct timespec __time;

    struct timespec cust_appeared;
    struct timespec order_baked;
    struct timespec order_packed;
    struct timespec order_delivered;
    struct timespec elapsed_time;

    struct tm timeInfo;

    thread_args_t args = *(thread_args_t *)__threadArgs;

    // if (pthread_mutex_lock(&mutexOut) != 0) {
    //     fprintf(stderr, "Thread (%d): Error locking mutexOut.\n", args.oid);
    //     free(__threadArgs);
    //     __threadArgs = NULL;
    //     pthread_exit(NULL);
    // }
    
    // getTime_r(&__time, &timeInfo, time_buffer);

    // printf("Delivery with number <%d>: Looking for telephone...: [%s]\n", args.oid, time_buffer);
    // pthread_mutex_unlock(&mutexOut);

    if (pthread_mutex_lock(&mutexTel) != 0) {
        fprintf(stderr, "Delivery with number <%d>: Error locking mutexTel.\n", args.oid);
        free(__threadArgs);
        __threadArgs = NULL;
        pthread_exit(NULL);
    }

    while (available_tel == 0) {
        pthread_cond_wait(&condFindTel, &mutexTel);
    }
    --available_tel;
    pthread_mutex_unlock(&mutexTel);

    getTime_r(&cust_appeared, &timeInfo, time_buffer);

    // if (pthread_mutex_lock(&mutexOut) != 0) {
    //     fprintf(stderr, "Delivery with number <%d>: Error locking mutexOut.\n", args.oid);
    //     free(__threadArgs);
    //     __threadArgs = NULL;
    //     pthread_exit(NULL);
    // }
    // printf("Delivery with number <%d>: Found available telephone at: [%s].\n", args.oid, time_buffer);
    // pthread_mutex_unlock(&mutexOut);

    int pizza_n = bounded_rand(T_ORDERLOW, T_ORDERHIGH, args.seed);
    int cust_total = 0;
    int cust_types_count[3] = { 0 };
    for (int i = 0; i < pizza_n; ++i) {
        int chance = rand_r(args.seed) % 100;
        int type = -1;
        if (chance <= P_S)
            type = PIZZA_SPECIAL;
        else if (P_S < chance && chance <= P_S + P_P)
            type = PIZZA_PEPERONI;
        else if (P_S + P_P < chance && chance <= 100)
            type = PIZZA_MARGARITA;

        ++cust_types_count[type];
        cust_total += pizza_cost[type];
    }

    int paymentTime = bounded_rand(T_PAYMENTLOW, T_PAYMENTHIGH, args.seed);
    sleep(paymentTime);

    getTime_r(&__time, &timeInfo, time_buffer);

    int fail_chance = rand_r(args.seed) % 100;

    if (pthread_mutex_lock(&mutexTel) != 0) {
        fprintf(stderr, "Delivery with number <%d>: Error locking mutexTel.\n", args.oid);
        free(__threadArgs);
        __threadArgs = NULL;
        pthread_exit(NULL);
    }

    ++available_tel;
    if(fail_chance <= P_FAIL){
        ++failedPayments;

        if (pthread_mutex_lock(&mutexOut) != 0) {
            fprintf(stderr, "Delivery with number <%d>: Error locking mutexOut.\n", args.oid);
            free(__threadArgs);
            __threadArgs = NULL;
            pthread_exit(NULL);
        }

        printf("Delivery with number <%d>: Payment failed at [%s].\n", args.oid, time_buffer);
        pthread_mutex_unlock(&mutexOut);

        free(__threadArgs);
        __threadArgs = NULL;

        pthread_cond_signal(&condFindTel);

        pthread_mutex_unlock(&mutexTel);
        pthread_exit(NULL);
    }
    total += cust_total;
    ++successfullPayments;

    for(int i = 0; i < 3; ++i)
        type_sales[i] += cust_types_count[i];

    pthread_cond_signal(&condFindTel);
    pthread_mutex_unlock(&mutexTel);

    if (pthread_mutex_lock(&mutexOut) != 0) {
        fprintf(stderr, "Delivery with number <%d>: Error locking mutexOut.\n", args.oid);
        free(__threadArgs);
        __threadArgs = NULL;
        pthread_exit(NULL);
    }
    printf("Delivery with number <%d>: Payment was successful, customer total: %d, [%s]\n", args.oid, cust_total, time_buffer);
    pthread_mutex_unlock(&mutexOut);

    if (pthread_mutex_lock(&mutexCook) != 0) {
        fprintf(stderr, "Delivery with number <%d>: Error locking mutexCook.\n", args.oid);
        free(__threadArgs);
        __threadArgs = NULL;

        pthread_exit(NULL);
    }

    while (available_cook == 0) {
        pthread_cond_wait(&condFindCook, &mutexCook);
    }

    --available_cook;
    pthread_mutex_unlock(&mutexCook);

    if (pthread_mutex_lock(&mutexOut) != 0) {
        fprintf(stderr, "Delivery with number <%d>: Error locking mutexOut.\n", args.oid);
        free(__threadArgs);
        __threadArgs = NULL;

        pthread_exit(NULL);
    }
    pthread_mutex_unlock(&mutexOut);

    sleep(pizza_n * T_PREP);

    if (pthread_mutex_lock(&mutexCook) != 0) {
        fprintf(stderr, "Delivery with number <%d>: Error locking mutexCook.\n", args.oid);
        free(__threadArgs);
        __threadArgs = NULL;

        pthread_exit(NULL);
    }
    ++available_cook;
    pthread_cond_signal(&condFindCook);
    pthread_mutex_unlock(&mutexCook);

    if (pthread_mutex_lock(&mutexOut) != 0) {
        fprintf(stderr, "Delivery with number <%d>: Error locking mutexOut.\n", args.oid);
        free(__threadArgs);
        __threadArgs = NULL;

        pthread_exit(NULL);
    }
    // printf("Delivery with number <%d>: Looking for %d ovens...\n", args.oid, pizza_n);
    pthread_mutex_unlock(&mutexOut);

    if (pthread_mutex_lock(&mutexOven) != 0) {
        fprintf(stderr, "Delivery with number <%d>: Error locking mutexOven.\n", args.oid);
        free(__threadArgs);
        __threadArgs = NULL;

        pthread_exit(NULL);
    }

    while (available_oven < pizza_n) {
        pthread_cond_wait(&condFindOven, &mutexOven);
    }

    available_oven -= pizza_n;
    pthread_mutex_unlock(&mutexOven);

    if (pthread_mutex_lock(&mutexOut) != 0) {
        fprintf(stderr, "Delivery with number <%d>: Error locking mutexOut.\n", args.oid);
        free(__threadArgs);
        __threadArgs = NULL;

        pthread_exit(NULL);
    }
    // printf("Delivery with number <%d>: %d Ovens found.\n", args.oid, pizza_n);
    // printf("Delivery with number <%d>: Available ovens: %d\n", args.oid, available_oven);
    // printf("Delivery with number <%d>: Cooking pizzas...\n", args.oid);
    pthread_mutex_unlock(&mutexOut);

    sleep(T_BAKE);
    clock_gettime(CLOCK_REALTIME, &order_baked);

    if (pthread_mutex_lock(&mutexOven) != 0) {
        fprintf(stderr, "Delivery with number <%d>: Error locking mutexOven.\n", args.oid);
        free(__threadArgs);
        __threadArgs = NULL;

        pthread_exit(NULL);
    }
    // printf("Delivery with number <%d>: Had ordered %d pizzas.\n", args.oid, pizza_n);
    // printf("Delivery with number <%d>: Avaiable ovens before increment: %d\n", args.oid, available_oven);
    available_oven += pizza_n;
    // printf("Delivery with number <%d>: Avaiable ovens after increment: %d\n", args.oid, available_oven);
    pthread_cond_broadcast(&condFindOven);
    pthread_mutex_unlock(&mutexOven);

    if (pthread_mutex_lock(&mutexDel) != 0) {
        fprintf(stderr, "Delivery with number <%d>: Error locking mutexDel.\n", args.oid);
        free(__threadArgs);
        __threadArgs = NULL;

        pthread_exit(NULL);
    }

    // printf("Delivery with number <%d>: Looking for delivery guy...\n", args.oid);
    while (available_delivery == 0) {
        pthread_cond_wait(&condFindDelivery, &mutexDel);
    }

    --available_delivery;
    pthread_mutex_unlock(&mutexDel);

    sleep(pizza_n * T_PACK);

    getTime_r(&order_packed, &timeInfo, time_buffer);
    elapsed_time = getTimeDiff(&cust_appeared, &order_packed);

    if (pthread_mutex_lock(&mutexOut) != 0) {
        fprintf(stderr, "Delivery with number <%d>: Error locking mutexOut.\n", args.oid);
        free(__threadArgs);
        __threadArgs = NULL;

        pthread_exit(NULL);
    }
    printf("Delivery with number <%d>: Got ready in <%ld> minutes at [%s].\n", args.oid, timespec_to_minutes(elapsed_time), time_buffer);
    // printf("Delivery with number <%d>: Delivery found.\n", args.oid);
    // printf("Delivery with number <%d>: Packing pizzas, it will take %d minutes.\n", args.oid, pizza_n * T_PACK);
    // printf("Delivery with number <%d>: Available delivery: %d\n", args.oid, available_delivery);
    pthread_mutex_unlock(&mutexOut);

    int delTime = bounded_rand(T_DELLOW, T_DELHIGH, args.seed);
    // printf("Delivery with number <%d>: Delivering pizzas...\n", args.oid);

    sleep(delTime);

    getTime_r(&order_delivered, &timeInfo, time_buffer);
    elapsed_time = getTimeDiff(&cust_appeared, &order_delivered);

    if (pthread_mutex_lock(&mutexOut) != 0) {
        fprintf(stderr, "Delivery with number <%d>: Error locking mutexOut.\n", args.oid);
        free(__threadArgs);
        __threadArgs = NULL;

        pthread_exit(NULL);
    }

    printf("Delivery with number <%d>: Pizzas delivered in <%ld> minutes at [%s].\n", args.oid, timespec_to_minutes(elapsed_time), time_buffer);
    pthread_mutex_unlock(&mutexOut);

    sleep(delTime);

    if (pthread_mutex_lock(&mutexDel) != 0) {
        fprintf(stderr, "Delivery with number <%d>: Error locking mutexDel.\n", args.oid);
        free(__threadArgs);
        __threadArgs = NULL;

        pthread_exit(NULL);
    }

    ++available_delivery;
    pthread_cond_signal(&condFindDelivery);

    if(compare_timespec(&max_serv_time, &elapsed_time) == -1)
        max_serv_time = elapsed_time;
    avg_serv_time = add_timespecs(avg_serv_time, elapsed_time);
    
    elapsed_time = getTimeDiff(&order_baked, &order_delivered);
    if(compare_timespec(&max_cold_time, &elapsed_time) == -1)
        max_cold_time = elapsed_time;
    avg_cold_time = add_timespecs(avg_cold_time, elapsed_time);

    pthread_mutex_unlock(&mutexDel);

    free(__threadArgs);
    __threadArgs = NULL;
    pthread_exit(NULL);
}