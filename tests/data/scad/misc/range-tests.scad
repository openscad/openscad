echo("[a01] ----- [1:4]");      for (a = [1:4])      echo ("[a01] ", a);
echo("[a02] ----- [4:1]");      for (a = [4:1])      echo ("[a02] ", a);
echo("[a03] ----- [0:0]");      for (a = [0:0])      echo ("[a03] ", a);
echo("[a04] ----- [0:3]");      for (a = [0:3])      echo ("[a04] ", a);
echo("[a05] ----- [-3:0]");     for (a = [-3:0])     echo ("[a05] ", a);
echo("[a06] ----- [0:-3]");     for (a = [0:-3])     echo ("[a06] ", a);
echo("[a07] ----- [-2:2]");     for (a = [-2:2])     echo ("[a07] ", a);
echo("[a08] ----- [2:-2]");     for (a = [2:-2])     echo ("[a08] ", a);

echo("[b01] ----- [1:1:5]");    for (a = [1:1:5])    echo ("[b01] ", a);
echo("[b02] ----- [1:2:5]");    for (a = [1:2:5])    echo ("[b02] ", a);
echo("[b03] ----- [1:-1:5]");   for (a = [1:-1:5])   echo ("[b03] ", a);
echo("[b04] ----- [5:1:1]");    for (a = [5:1:1])    echo ("[b04] ", a);
echo("[b05] ----- [5:2:1]");    for (a = [5:2:1])    echo ("[b05] ", a);
echo("[b06] ----- [5:-1:1]");   for (a = [5:-1:1])   echo ("[b06] ", a);
echo("[b07] ----- [0:0:0]");    for (a = [0:0:0])    echo ("[b07] ", a);
echo("[b08] ----- [1:0:1]");    for (a = [1:0:1])    echo ("[b08] ", a);
echo("[b09] ----- [1:0:5]");    for (a = [1:0:5])    echo ("[b09] ", a);
echo("[b10] ----- [0:1:0]");    for (a = [0:1:0])    echo ("[b10] ", a);
echo("[b11] ----- [3:-.5:-3]"); for (a = [3:-.5:-3]) echo ("[b11] ", a);

// Check precision of fractional step sizes
// Could check up to 9999 (max allowable range size), but takes >10s
for (d=[1:1000]) 
  let(l=len([for(x=[0:1/d:1]) 0])) 
    if (l != d+1) echo(d,l);
