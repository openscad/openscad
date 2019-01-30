ver=version();
now=datetime();

//echo(ver);
//echo(now);

echo("current date time should be greater then version")
if(ver[0]<now[0]){
    //OK - nothing more to compare
}else if(ver[0]==now[0]){
    if(ver[1]<now[1]){
    //OK - nothing more to compare
    }else if(ver[1]==now[1]){
        if(ver[2]<=now[2]){
            //OK - nothing more to compare
        }else{
            echo("Current day smaller then build day");
            echo(ver[2], now[2]);
        }
    }else{
        echo("Current month smaller then build month");
        echo(ver[1], now[1]);
    }
}else{
    echo("Current year smaller then build year");
    echo(ver[0], now[0]);
}

echo("check the range");
if(2018>now[0]){
    echo("year before 2018:", now[0]);
}
if(1>now[1] || 12<now[1]){
    echo("month out of range:", vnow[1]);
}
if(1>now[2] || 31<now[2]){
    echo("day out of range:", now[2]);
}
if(0>now[3] || 24<now[3]){
    echo("hour out of range:", now[3]);
}
if(0>now[4] || 59<now[4]){
    echo("minute out of range:", now[4]);
}
if(0>now[5] || 60<now[5]){ //60 is the leap second
    echo("second out of range:", now[5]);
}

echo("end of datetime tests");