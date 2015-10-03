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
  Fermat f;
  f.whoAmI();
  f.whoAmIReally();

  Erdos& e = f;
  e.whoAmI();
  e.whoAmIReally();
}
