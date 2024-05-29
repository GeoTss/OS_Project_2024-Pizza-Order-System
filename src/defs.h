#define N_TEL 2
#define N_COOK 2
#define N_OVEN 10
#define N_DELIVERY 3
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

enum pizza_type{PIZZA_MARGARITA, PIZZA_PEPERONI, PIZZA_SPECIAL};
const int pizza_cost[] = {P_M, P_P, P_S};
const char* pizza_names[] = {"Margarita", "Peperoni", "Special"};

enum error_codes{SUCCESS, ARGUMENT_ERROR, MEMORY_ALLOC_ERROR, MUTEX_INIT_ERROR, COND_INIT_ERROR, THREAD_CREATE_ERROR, THREAD_JOIN_ERROR, MUTEX_DESTROY_ERROR, COND_DESTROY_ERROR};