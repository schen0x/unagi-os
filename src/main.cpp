extern "C" int KernelMain() {
  volatile int i = 10;
  for (; i < 100000; i++) {
    asm("pause");
  }
  asm("hlt");
  return 42;
}
