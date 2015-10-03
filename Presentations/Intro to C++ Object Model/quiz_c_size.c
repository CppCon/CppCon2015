#include <stdio.h>

typedef struct
{
  float real;
  float imag;
} Complex;

int main()
{
  printf("sizeof(float): %ld\n", sizeof(float));
  printf("sizeof(Complex): %ld\n", sizeof(Complex));
}
