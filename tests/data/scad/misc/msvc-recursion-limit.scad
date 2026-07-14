// Purpose: trigger MSVC-specific recursion counter without exhausting stack
module limited(n) {
  if (n > 0)
    limited(n - 1);
}
limited(1100);
