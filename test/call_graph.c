//
// Created by milica on 26.7.23..
//

#include <stdio.h>

void g()
{
  printf("Hello!\n");
}

void f()
{
  g();
}

int main (int argc, char** argv)
{
  f();
  return 0;
}