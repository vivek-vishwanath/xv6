#include "fs.h"
#include "sched.h"
#include "stat.h"
#include "types.h"
#include "user.h"

int
main (int argc, char *argv[])
{
  int matrix_size = atoi (argv[1]);
  // printf(1, "matrix multiply benchmark: %d\n", matrix_size);

  int *matrix_a = (int *)malloc (matrix_size * matrix_size * sizeof (int));
  int *matrix_b = (int *)malloc (matrix_size * matrix_size * sizeof (int));
  int *matrix_c = (int *)malloc (matrix_size * matrix_size * sizeof (int));
  memset (matrix_c, 0, matrix_size * matrix_size * sizeof (int));

  for (int m = 0; m < matrix_size * matrix_size; m++)
    {
      matrix_a[m] = m;
      matrix_b[m] = m;
    }
  for (int i = 0; i < matrix_size; i++)
    {
      for (int k = 0; k < matrix_size; k++)
        {
          for (int j = 0; j < matrix_size; j++)
            {
              matrix_c[i + j * matrix_size] += matrix_a[i + k * matrix_size]
                                               + matrix_b[k + j * matrix_size];
            }
        }
    }
  free (matrix_a);
  free (matrix_b);
  free (matrix_c);
  exit ();
}