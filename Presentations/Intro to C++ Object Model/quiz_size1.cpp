#include <iostream>

struct Complex
{
  float real;
  float imag;
};

int main()
{
  std::cout<<"sizeof(float): "<<sizeof(float)<<"\n";
  std::cout<<"sizeof(Complex): "<<sizeof(Complex)<<"\n";
}