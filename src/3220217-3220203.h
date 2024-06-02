/*
    Γεώργιος Τσοράκος - 3220217
    Φίλιππος Τοτόμης  - 3220203
*/

#define N_TEL 2
#define N_COOK 2
#define N_OVEN 10
#define N_DELIVERY 10
#define T_ORDERLOW 1
#define T_ORDERHIGH 5
#define N_ORDERLOW 1
#define N_ORDERHIGH 5
#define P_M 35
#define P_P 25
#define P_S 40
#define T_PAYMENTLOW 1
#define T_PAYMENTHIGH 3
#define P_FAIL 5
#define C_M 10
#define C_P 11
#define C_S 12
#define T_PREP 1
#define T_BAKE 10
#define T_PACK 1
#define T_DELLOW 5
#define T_DELHIGH 15

#define NANOSECONDS_PER_SECOND 1000000000L
#define SECONDS_PER_MINUTE 60

enum pizza_type{PIZZA_MARGARITA, PIZZA_PEPERONI, PIZZA_SPECIAL};
const int pizza_cost[] = {P_M, P_P, P_S};
const char* pizza_names[] = {"Margarita", "Peperoni", "Special"};

enum error_codes{SUCCESS, ARGUMENT_ERROR, MEMORY_ALLOC_ERROR, MUTEX_INIT_ERROR, COND_INIT_ERROR, THREAD_CREATE_ERROR, THREAD_JOIN_ERROR, MUTEX_DESTROY_ERROR, COND_DESTROY_ERROR};

typedef struct thread_args{
    int oid;
    unsigned int seed;
} thread_args_t;

int bounded_rand(int, int, unsigned int*);
void clean_up(int, int);
static inline void getTime_r(struct timespec*, struct tm*, char* buffer);
static inline struct timespec getTimeDiff(struct timespec*, struct timespec*);
int compare_timespec(struct timespec*, struct timespec*);
time_t timespec_to_minutes(struct timespec);
struct timespec add_timespecs(struct timespec, struct timespec);
void *order(void *);