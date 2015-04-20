//Test search with unicode strings

//Helper function that pretty prints our search test
//Expected result is checked against execution of a search() invocation and OK/FAIL is indicated
module test_search_and_echo( exp_res, search_to_find, search_to_search) {
    test_res = search(search_to_find, search_to_search);
    echo(str("Expect ", exp_res, " for search(", search_to_find, ", ", search_to_search, ")=", test_res, ". ", (exp_res == test_res)?"OK":"FAIL"  ));
}


//"Normal" text for comparison
echo ("----- Lookup of 1 byte into 1 byte");
//Hits - up_to_count 1
test_search_and_echo( [0,1,2,3],   "a","aaaa" );
test_search_and_echo( [0,1,2], "aa","aaaa" );

//Misses
test_search_and_echo( undef,		"b","aaaa" );

test_search_and_echo( undef,			"bb","aaaa" );
//Miss - empties
test_search_and_echo( undef, "","aaaa" );
test_search_and_echo( undef, "","" );
test_search_and_echo( undef, "a","" );


//Unicode tests
echo ("----- Lookup of multi-byte into 1 byte");
test_search_and_echo( undef,		"Л","aaaa" );
test_search_and_echo( undef,		"🂡","aaaa" );

test_search_and_echo( undef,		"ЛЛ","aaaa" );
test_search_and_echo( undef,		"🂡🂡","aaaa" );

echo ("----- Lookup of 1-byte into multi-byte");
test_search_and_echo( undef , "a","ЛЛЛЛ" );
test_search_and_echo( undef , "a","🂡🂡🂡🂡" );

echo ("----- Lookup of 1-byte into mixed multi-byte");
test_search_and_echo( [0,2,4,6,8], "a","aЛaЛaЛaЛa" );
test_search_and_echo( [0,2,4,6,8], "a","a🂡a🂡a🂡a🂡a" );
test_search_and_echo( [0,4,8], "a","a🂡Л🂡a🂡Л🂡a" );

test_search_and_echo( [0,2,4,6,8], "a","aЛaЛaЛaЛa");
test_search_and_echo( [0,2,4,6,8], "a","a🂡a🂡a🂡a🂡a");
test_search_and_echo( [0,4,8]    , "a","a🂡Л🂡a🂡Л🂡a");

echo ("----- Lookup of 2-byte into 2-byte");
test_search_and_echo( [0,1,2,3]       , "Л","ЛЛЛЛ" );

echo ("----- Lookup of 2-byte into 4-byte");
test_search_and_echo( undef , "Л","🂡🂡🂡🂡" );

echo ("----- Lookup of 4-byte into 4-byte");
test_search_and_echo( [0,1,2,3] , 		  "🂡","🂡🂡🂡🂡" );

echo ("----- Lookup of 4-byte into 2-byte");
test_search_and_echo( undef , "🂡","ЛЛЛЛ" );

echo ("----- Lookup of 2-byte into mixed multi-byte");
test_search_and_echo( [1,3,5,7] , 	"Л","aЛaЛaЛaЛa");
test_search_and_echo( undef , 	"Л","a🂡a🂡a🂡a🂡a");
test_search_and_echo( [2,6] , 	"Л","a🂡Л🂡a🂡Л🂡a");

echo ("----- Lookup of 4-byte into mixed multi-byte");
test_search_and_echo( undef , 			"🂡","aЛaЛaЛaЛa");
test_search_and_echo( [1,3,5,7] , "🂡","a🂡a🂡a🂡a🂡a");

echo ("----- Lookup of mixed multi-byte into mixed multi-byte");
test_search_and_echo( [4], "aЛ🂡","aЛaЛaЛ🂡aЛaЛa");
test_search_and_echo( [4], "aЛ🂡","a🂡a🂡aЛ🂡a🂡a🂡a");
test_search_and_echo( [4]    , "aЛ🂡","a🂡Л🂡aЛ🂡a🂡Л🂡a");
test_search_and_echo( [4]    , "🂡aЛ","a🂡Л🂡🂡aЛa🂡Л🂡a");

