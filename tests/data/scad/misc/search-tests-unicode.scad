//Test search with unicode strings

//Helper function that pretty prints our search test
//Expected result is checked against execution of a search() invocation and OK/FAIL is indicated
module test_search_and_echo( exp_res, search_to_find, search_to_search, search_up_to_num_matches = undef) {
   if (undef != search_up_to_num_matches) {
      test_res = search(search_to_find, search_to_search, search_up_to_num_matches);
      echo(str("Expect ", exp_res, " for search(", search_to_find, ", ", search_to_search, ", ", search_up_to_num_matches, ")=", test_res, ". ", (exp_res == test_res)?"OK":"FAIL"  ));
   }
   else {
      test_res = search(search_to_find, search_to_search);
      echo(str("Expect ", exp_res, " for search(", search_to_find, ", ", search_to_search, ")=", test_res, ". ", (exp_res == test_res)?"OK":"FAIL"  ));
   }
}


//"Normal" text for comparison
echo ("----- Lookup of 1 byte into 1 byte");
//Hits - up_to_count 1
test_search_and_echo( [0],   "a","aaaa" );
test_search_and_echo( [0],   "a","aaaa",1 );
test_search_and_echo( [0,0], "aa","aaaa" );
test_search_and_echo( [0,0], "aa","aaaa",1 );


//Hits - up to count 1+ (incl 0 == all)
test_search_and_echo( [[0,1,2,3]] , 	"a","aaaa",0 );
test_search_and_echo( [[0,1]], 			"a","aaaa",2 );
test_search_and_echo( [[0,1,2]], 		"a","aaaa",3 );
test_search_and_echo( [[0,1,2,3]] , 	"a","aaaa",4 );
test_search_and_echo( [[0,1,2,3],[0,1,2,3]] , "aa","aaaa",0 );
//Misses
test_search_and_echo( [],		"b","aaaa" );
test_search_and_echo( [],		"b","aaaa",1 );
test_search_and_echo( [[]],		"b","aaaa",0 );
test_search_and_echo( [[]],		"b","aaaa",2 );

test_search_and_echo( [],			"bb","aaaa" );
test_search_and_echo( [],			"bb","aaaa",1 );
test_search_and_echo( [[],[]],		"bb","aaaa",0 );
test_search_and_echo( [[],[]],		"bb","aaaa",2 );
//Miss - empties
test_search_and_echo( [], "","aaaa" );
test_search_and_echo( [], "","" );
test_search_and_echo( [], "a","" );


//Unicode tests
echo ("----- Lookup of multi-byte into 1 byte");
test_search_and_echo( [],		"Л","aaaa" );
test_search_and_echo( [],		"🂡","aaaa" );
test_search_and_echo( [[]],		"Л","aaaa",0 );
test_search_and_echo( [[]],		"🂡","aaaa",0 );

test_search_and_echo( [],		"ЛЛ","aaaa" );
test_search_and_echo( [],		"🂡🂡","aaaa" );
test_search_and_echo( [[],[]],		"ЛЛ","aaaa",0 );
test_search_and_echo( [[],[]],		"🂡🂡","aaaa",0 );

echo ("----- Lookup of 1-byte into multi-byte");
test_search_and_echo( [] , "a","ЛЛЛЛ" );
test_search_and_echo( [] , "a","🂡🂡🂡🂡" );
test_search_and_echo( [] , "a","ЛЛЛЛ",1 );

test_search_and_echo( [[]] , "a","🂡🂡🂡🂡",0 );
test_search_and_echo( [[]] , "a","🂡🂡🂡🂡",2 );

echo ("----- Lookup of 1-byte into mixed multi-byte");
test_search_and_echo( [0], "a","aЛaЛaЛaЛa" );
test_search_and_echo( [0], "a","a🂡a🂡a🂡a🂡a" );
test_search_and_echo( [0], "a","a🂡Л🂡a🂡Л🂡a" );

test_search_and_echo( [[0,2,4,6,8]], "a","aЛaЛaЛaЛa",0 );
test_search_and_echo( [[0,2,4,6,8]], "a","a🂡a🂡a🂡a🂡a", 0 );
test_search_and_echo( [[0,4,8]]    , "a","a🂡Л🂡a🂡Л🂡a", 0 );

echo ("----- Lookup of 2-byte into 2-byte");
test_search_and_echo( [0]       , "Л","ЛЛЛЛ" );
test_search_and_echo( [[0,1,2,3]] , "Л","ЛЛЛЛ",0 );

echo ("----- Lookup of 2-byte into 4-byte");
test_search_and_echo( [] , "Л","🂡🂡🂡🂡" );

echo ("----- Lookup of 4-byte into 4-byte");
test_search_and_echo( [0] , 		  "🂡","🂡🂡🂡🂡" );
test_search_and_echo( [[0,1,2,3]], "🂡","🂡🂡🂡🂡",0 );

echo ("----- Lookup of 4-byte into 2-byte");
test_search_and_echo( [] , "🂡","ЛЛЛЛ" );

echo ("----- Lookup of 2-byte into mixed multi-byte");
test_search_and_echo( [1] , 	"Л","aЛaЛaЛaЛa",1 );
test_search_and_echo( [] , 	"Л","a🂡a🂡a🂡a🂡a", 1 );
test_search_and_echo( [2] , 	"Л","a🂡Л🂡a🂡Л🂡a", 1 );

test_search_and_echo( [[1,3,5,7]] , 	"Л","aЛaЛaЛaЛa",0 );
test_search_and_echo( [[]] , 				"Л","a🂡a🂡a🂡a🂡a", 0 );
test_search_and_echo( [[2,6]] , 			"Л","a🂡Л🂡a🂡Л🂡a", 0 );

echo ("----- Lookup of 4-byte into mixed multi-byte");
test_search_and_echo( [] , 			"🂡","aЛaЛaЛaЛa",1 );
test_search_and_echo( [1] , "🂡","a🂡a🂡a🂡a🂡a", 1 );

test_search_and_echo( [[]] , 			"🂡","aЛaЛaЛaЛa",0 );
test_search_and_echo( [[1,3,5,7]] , "🂡","a🂡a🂡a🂡a🂡a", 0 );
test_search_and_echo( [[1,3,5,7]] , "🂡","a🂡Л🂡a🂡Л🂡a", 0 );

echo ("----- Lookup of mixed multi-byte into mixed multi-byte");
test_search_and_echo( [[0,2,4,6,8],[1,3,5,7],[]], "aЛ🂡","aЛaЛaЛaЛa",0 );
test_search_and_echo( [[0,2,4,6,8],[],[1,3,5,7]], "aЛ🂡","a🂡a🂡a🂡a🂡a", 0 );
test_search_and_echo( [[0,4,8],[2,6],[1,3,5,7]]    , "aЛ🂡","a🂡Л🂡a🂡Л🂡a", 0 );
test_search_and_echo( [[1,3,5,7],[0,4,8],[2,6]]    , "🂡aЛ","a🂡Л🂡a🂡Л🂡a", 0 );

