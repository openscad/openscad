data = import("../../json/data.json");
echo(data);

echo(data.string); // ECHO: "hallo world!"
echo(data.array_number); // ECHO: [0, 1, 2, 3, 5]
echo(data["array-string"]); // ECHO: ["one", "two", "three"]
echo(data.object.name); // ECHO: "The object name"
echo(data.object.nested.value); // ECHO: 42

// Test an import of a file with a non-ASCII name.
echo(import("../../json/☠-data.json"));

// Test file-type checking
// A JSON by any other name would smell as sweet.
// Test that an unknown extension fails.
echo(import("../../json/data.rose"));
// Test that an unknown extension succeeds when you explicitly specify a known type.
echo(import("../../json/data.rose", type="json"));
// Test that a known extension fails when you explicitly specify an unknown type.
echo(import("../../json/data.json", type="rose"));


// Test that a file with no extension fails.
echo(import("../../json/data"));
// Test that a file with no extension succeeds when you explicitly specify a known type.
echo(import("../../json/data", type="json"));
