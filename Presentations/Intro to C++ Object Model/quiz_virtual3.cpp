#include <iostream>
using std::cout;

struct Erdos
{
  void whoAmI() { cout<<"I am Erdos\n"; }
  virtual void whoAmIReally() { cout<<"I really am Erdos\n"; }
};

struct Fermat : public Erdos
{
  void whoAmI() { cout<<"I am Fermat\n"; }
  virtual void whoAmIReally() { cout<<"I really am Fermat\n"; }
};

int main()
{
  Erdos * e1 = new Erdos;
  e1->whoAmI();
  e1->whoAmIReally();

  Erdos * e2 = new Fermat;
  e2->whoAmI();
  e2->whoAmIReally();
}
