function f(x) = echo(x) x;

// associativity
echo(assoc_right_unary = 4---3);

echo(assoc_left_addsub = f(2) - f(3) + f(4));
echo(assoc_left_subadd = f(2) + f(3) - f(4));
echo(assoc_left_muldiv = f(2) * f(3) / f(4));
echo(assoc_left_ltgt   = f(3) < f(4) > f(5));
echo(assoc_left_eqne   = f(true) == f(true) != f(false));
echo(assoc_left_and    = f(true) && f(true) && f(false));
echo(assoc_left_or     = f(true) || f(true) || f(false));

// precedence
echo(prec_andor = true && true || true && false);
echo(prec_orand = false || false && false || true);

echo(prec_gtadd = 3 + 2 > 4);
echo(prec_addgt = 5 > 2 + 4);

echo(prec_addmul = 2 + 3 * 4);
echo(prec_muladd = 2 * 3 + 4);
echo(prec_submul = 2 - 3 * 4);
echo(prec_mulsub = 2 * 3 - 4);
echo(prec_addmod = 2 + 3 % 4);
echo(prec_modadd = 2 % 3 + 4);

echo(prec_unarysub = 2 / -2 / 2);
echo(prec_unaryadd = 2 / +2 / 2);
