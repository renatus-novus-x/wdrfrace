#include "app.h"

int main(void) {
  Application application;
  if (!application.initialize()) return 0;
  while (application.update()) application.render();
  application.finalize();
  return 0;
}
