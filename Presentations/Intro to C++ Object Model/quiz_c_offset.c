#include <stdio.h>

typedef struct
{
  float real;
  float imag;
} Complex;

int main()
{
  Complex c;
  printf("address of c: %p\n", &c);
  printf("address of c.real: %p\n", &c.real);
  printf("address of c.imag: %p\n", &c.imag);
}
