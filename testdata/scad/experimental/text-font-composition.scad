use <../../ttf/liberation-2.00.1/LiberationSans-Regular.ttf>

// Validate correct behavior with overlapping combining chars.
// The U+030A (COMBINING RING ABOVE) should be unioned to the
// letter and not generate a hole where the A and the ring
// overlap.
text(text = "A\u030a", font = "Liberation Sans", size = 40);
