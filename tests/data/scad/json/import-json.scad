data = import("../../json/data.json");
echo(data);

echo(data.string); // ECHO: "hallo world!"
echo(data.array_number); // ECHO: [0, 1, 2, 3, 5]
echo(data["array-string"]); // ECHO: ["one", "two", "three"]
echo(data.object.name); // ECHO: "The object name"
echo(data.object.nested.value); // ECHO: 42

// Test an import of a file with a non-ASCII name.
echo(import("../../json/☠-data.json"));
