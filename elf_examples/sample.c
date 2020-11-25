#include <stdio.h>

char *hello = "Hello";
char *who = "world";

char *whois(void) {
   return who;
}
char *what(void) {
   return hello;
}
int main (int argc, char *argv[]) {

  printf("%s %s\n", what(), whois());
  return 0;
}
