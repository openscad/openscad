//Test length reporting
text_1bytes_len = "1234";
text_2bytes_len = "ЛЛЛЛ";
text_4bytes_len = "🂡🂱🃁🃑";

echo( "text_1bytes_len = ", text_1bytes_len, " len = ", len(text_1bytes_len)  );
echo( "text_2bytes_len = ", text_2bytes_len, " len = ", len(text_2bytes_len)  );
echo( "text_4bytes_len = ", text_4bytes_len, " len = ", len(text_4bytes_len)  );

//Test how well arrays of unicode string are accessed.

texts_array = [
"DEADBEEF",
"Ленивый рыжий кот",
"كسول الزنجبيل القط",
"懶惰的姜貓",
"äöü ÄÖÜ ß",
"😁😂😃😄😅😆😇😈😉😊😋😌😍😎😏😐",
"⠁⠂⠃⠄⠅⠆⠇⠈⠉⠊⠋⠌⠍⠎⠏",
"🂡🂱🃁🃑",
];

text_2bytes = "Ленивый рыжий кот";
text_4bytes = "🂡🂱🃁🃑";


//Test all the normal accesses
for (text_array_idx = [0:(len(texts_array)-1)])
{
	echo( "[", text_array_idx, "] = ", texts_array[text_array_idx], " of len=", len(texts_array[text_array_idx]), ":"  );
    for (text_idx = [0:(len(texts_array[text_array_idx])-1)])
    {
	    echo( "    [", text_idx, "]=", texts_array[text_array_idx][text_idx]  );
    }
}

//Test one past the last element of (x-byte unicode). This will be one past the length but inside the char length of the string
echo( "Past end of unicode only 2-byte ", text_2bytes[len(text_2bytes)]  );
echo( "Past end of unicode only 4-byte ", text_4bytes[len(text_4bytes)]  );

//Test past the last element of (x-byte unicode). Outside both lengths.
echo( "Past end of both 2-byte ", text_2bytes[ len(text_2bytes) * 2 ]   );
echo( "Past end of both 4-byte ", text_4bytes[ len(text_4bytes) * 4 ]   );

