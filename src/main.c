#include <stdio.h>

#include <starpu.h>

#define SIZE 10

void copy_function(void *buffers[], void *arguments) {
  printf("INFO\tcopy_function\n");

  int *a = STARPU_VECTOR_GET_PTR(buffers[0]);
  int *b = STARPU_VECTOR_GET_PTR(buffers[1]);

  for (int i = 0; i < SIZE; ++i)
    a[i] = b[i];
}
struct starpu_codelet copy_codelet = {.cpu_funcs = {copy_function},
                                      .cpu_funcs_name = {"copy_function"},
                                      .nbuffers = 2};

void add_function(void *buffers[], void *arguments) {
  printf("INFO\tadd_function\n");

  int *a = STARPU_VECTOR_GET_PTR(buffers[0]);
  int *b = STARPU_VECTOR_GET_PTR(buffers[1]);

  for (int i = 0; i < SIZE; ++i)
    a[i] += b[i];
}
struct starpu_codelet add_codelet = {
    .cpu_funcs = {add_function},
    .cpu_funcs_name = {"add_function"},
    .nbuffers = 2,
    .modes = {STARPU_RW | STARPU_COMMUTE, STARPU_R},
};

void zero_function(void *buffers[], void *arguments) {
  printf("INFO\tzero_codelet\n");

  int *a = STARPU_VECTOR_GET_PTR(buffers[0]);

  for (int i = 0; i < SIZE; ++i)
    a[i] = 0;
}

struct starpu_codelet zero_codelet = {
    .cpu_funcs = {zero_function},
    .cpu_funcs_name = {"zero_function"},
    .nbuffers = 1,
    .modes = {STARPU_W},
};

int main(void) {
  int a[SIZE];
  int b[SIZE];
  int c[SIZE];

  for (int i = 0; i < SIZE; ++i) {
    a[i] = 0;
    b[i] = 1;
    c[i] = 2;
  }

  for (int i = 0; i < SIZE; ++i)
    printf("(%d, %d, %d)\n", a[i], b[i], c[i]);

  starpu_init(NULL);

  starpu_data_handle_t a_handle_1;
  starpu_data_handle_t a_handle_2;
  starpu_data_handle_t b_handle;
  starpu_data_handle_t c_handle;

  starpu_vector_data_register(&a_handle_1, STARPU_MAIN_RAM, (uintptr_t)a, SIZE,
                              sizeof(int));
  starpu_vector_data_register(&a_handle_2, STARPU_MAIN_RAM, (uintptr_t)a, SIZE,
                              sizeof(int));
  starpu_vector_data_register(&b_handle, STARPU_MAIN_RAM, (uintptr_t)b, SIZE,
                              sizeof(int));
  starpu_vector_data_register(&c_handle, STARPU_MAIN_RAM, (uintptr_t)c, SIZE,
                              sizeof(int));

  starpu_data_set_reduction_methods(a_handle_1, &add_codelet, &zero_codelet);
  starpu_data_set_reduction_methods(a_handle_2, &add_codelet, &zero_codelet);

  starpu_task_insert(&copy_codelet,            //
                     STARPU_REDUX, a_handle_1, //
                     STARPU_R, b_handle,       //
                     0);                       //

  starpu_task_insert(&copy_codelet,            //
                     STARPU_REDUX, a_handle_2, //
                     STARPU_R, c_handle,       //
                     0);                       //

  starpu_task_wait_for_all();

  starpu_data_unregister(a_handle_1);
  starpu_data_unregister(a_handle_2);
  starpu_data_unregister(b_handle);
  starpu_data_unregister(c_handle);

  starpu_shutdown();

  for (int i = 0; i < SIZE; ++i)
    printf("(%d, %d, %d)\n", a[i], b[i], c[i]);

  return 0;
}