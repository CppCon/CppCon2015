#include <iostream>
using std::cout;

struct Complex
{
  float real;
  float imag;
};

struct Derived : public Complex
{
  float angle;
};

int main()
{
  cout<<"sizeof(float): "<<sizeof(float)<<"\n";
  cout<<"sizeof(Complex): "<<sizeof(Complex)<<"\n";
  cout<<"sizeof(Derived): "<<sizeof(Derived)<<"\n";
}