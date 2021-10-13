// Each 'let' creates a new context, allowing variable override
function duplicate_let() = let(x=42) let(x=33) x;
echo(duplicate_let=duplicate_let()); // 33

// Default value (x=42) must be used, if not given in call
function defaults(b, x=42) = b ? x : defaults(true);
echo(defaults=defaults(false, 33)); // 42

// Regular local variables should not survive to the next tail recursion call
function scope_leak(b=false) = b ? x : let(x=42) scope_leak(true);
echo(scope_leak=scope_leak()); // undef

// ...however, "config variables" should
function scope_leak_config(b=false) = b ? $x : let($x=33) let($x=42) scope_leak_config(true);
echo(scope_leak_config=scope_leak_config()); // 42
