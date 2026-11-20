// Deep recursion ending in a failed assertion. The number of TRACE
// lines in the output is limited by --trace-depth.
module recurse(n) {
  if (n == 0) assert(false, "bottom reached");
  else recurse(n - 1);
}

recurse(20);
