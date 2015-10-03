#include <iostream>
using std::cout;

class Complex
{
  float real;
  float imag;
};

int main()
{
  cout<<"sizeof(float): "<<sizeof(float)<<"\n";
  cout<<"sizeof(Complex): "<<sizeof(Complex)<<"\n";
}