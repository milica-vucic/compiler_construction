//
// Created by milica on 26.7.23..
//

#include <stdio.h>

int main (int argc, char** argv)
{
  int x = 5, y = 3;

  if (x == 5) {
    printf("x je 5");
  } else {
    printf("x je razlicito od 5");
  }

  switch (y) {
  case 1:
    printf("1\n");
    break;
  case 2:
    printf("2\n");
    break;
  case 3:
    printf("3\n");
    break;
  default:
    printf("y je razlicito od 1, 2 i 3\n");
  }

  return 0;
}