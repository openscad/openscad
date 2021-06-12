function nop(x) = 0;

function works() =
  let (
    arr = [for (c = [0,0,0,0])        0],
    x = [for (e = arr) nop(arr[0])]
  ) 1;

// Use of multiple iterators "c" and "i" cause results to go into
// EmbeddedVectorTypes, which become flattened upon first use of operator[].
function save_by_pre_flattening() =
  let (
    arr = [for (c = [0,0], i = [0,0]) 0],
    tmp = arr[0],
    x = [for (e = arr) nop(arr[0])]
  ) 2;

// If this flattening occurs inside the loop, iterator e would be invalidated,
// so this special case is now also handled by the VectorType::iterator class.
function fails() =
  let (
    arr = [for (c = [0,0], i = [0,0]) 0],
    x = [for (e = arr) nop(arr[0])]
  ) 3;

// Parser splits multiple iterators into individual LcFor expressions.
// So fails and fails2 should be equivalent.
function fails2() =
  let (
    arr = [for (c = [0,0]) for(i = [0,0]) 0],
    x = [for (e = arr) nop(arr[0])]
  ) 4;

echo(works());
echo(save_by_pre_flattening());
echo(fails());
echo(fails2());
