// Test searching strings against a multi-column table (num_returns_per_match=1, index_col_num=1)

module testEq(a, b, s) {
  if (a != b) {
    echo("FAIL:", a, "==", b, s);
  } else {
    echo("PASS:", s);
  }
}

table = [
  [32, " ", "space"],
  [33, "!", "excl"],
  [49, "1", "one"],
  [50, "2", "two"],
  [51, "3", "three"],
  [65, "A", "upper-A"],
  [66, "B", "upper-B"],
  [67, "C", "upper-C"],
  [68, "D", "upper-D"],
  [79, "O", "upper-O"],
  [82, "R", "upper-R"],
  [83, "S", "upper-S"],
  [99, "c", "lower-c"],
  [101, "e", "lower-e"],
  [107, "k", "lower-k"],
  [110, "n", "lower-n"],
  [111, "o", "lower-o"],
  [112, "p", "lower-p"],
  [115, "s", "lower-s"]
];

text1 = "OpenSCAD Rocks!";
indices1 = search(text1, table, 1, 1);
testEq(indices1, [9, 17, 13, 15, 11, 7, 5, 8, 0, 10, 16, 12, 14, 18, 1], "search indices for 'OpenSCAD Rocks!'");
testEq([for (i = indices1) table[i][0]], [for (c = text1) ord(c)], "character code lookup for 'OpenSCAD Rocks!'");
testEq(chr([for (i = indices1) table[i][0]]), text1, "round-trip string from table lookup");

indices2 = search("ABC", table, 1, 1);
testEq(indices2, [5, 6, 7], "search indices for 'ABC'");
testEq([for (i = indices2) table[i][2]], ["upper-A", "upper-B", "upper-C"], "table value lookup for 'ABC'");

indices3 = search("123", table, 1, 1);
testEq(indices3, [2, 3, 4], "search indices for '123'");
testEq([for (i = indices3) table[i][2]], ["one", "two", "three"], "table value lookup for '123'");

