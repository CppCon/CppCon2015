#include <iostream>
#include <cmath>
using std::cout;

struct Complex
{
  float Abs() const { return std::hypot(real, imag); }
  float real;
  float imag;
};

int main()
{
  cout<<"sizeof(float): "<<sizeof(float)<<"\n";
  cout<<"sizeof(Complex): "<<sizeof(Complex)<<"\n";
}