#include "TestScadLexerFolding.h"

#include <QList>
#include <QTest>

#include "gui/ScintillaEditor.h"

namespace {

int lineCount(QsciScintilla *qsci)
{
  return qsci->SendScintilla(QsciScintilla::SCI_GETLINECOUNT);
}

int rawFoldLevel(QsciScintilla *qsci, int line)
{
  return qsci->SendScintilla(QsciScintilla::SCI_GETFOLDLEVEL, line);
}

// Fold depth of 'line', relative to 'baseLine' (line 0 by default).
// Comparing relative to a baseline -- rather than asserting an absolute
// depth -- avoids depending on Scintilla's internal SC_FOLDLEVELBASE
// offset, which ScadLexer2::fold() itself never hardcodes either; it
// only ever reasons about level *deltas*.
int foldDepth(QsciScintilla *qsci, int line, int baseLine = 0)
{
  const int base = rawFoldLevel(qsci, baseLine) & QsciScintilla::SC_FOLDLEVELNUMBERMASK;
  const int level = rawFoldLevel(qsci, line) & QsciScintilla::SC_FOLDLEVELNUMBERMASK;
  return level - base;
}

bool isFoldHeader(QsciScintilla *qsci, int line)
{
  return rawFoldLevel(qsci, line) & QsciScintilla::SC_FOLDLEVELHEADERFLAG;
}

QList<int> currentDepths(QsciScintilla *qsci)
{
  auto depths = QList<int>{};

  for (auto line = 0; line < lineCount(qsci); ++line) {
    depths += foldDepth(qsci, line);
  }

  return depths;
};

QList<int> currentHeaders(QsciScintilla *qsci)
{
  auto headers = QList<int>{};

  for (auto line = 0; line < lineCount(qsci); ++line) {
    if (isFoldHeader(qsci, line)) {
      headers += line;
    }
  }

  return headers;
};

}  // namespace

void TestScadLexerFolding::testFolding_data()
{
  QTest::addColumn<QString>("source");
  QTest::addColumn<QList<int>>("expectedDepths");   // one entry per line
  QTest::addColumn<QList<int>>("expectedHeaders");  // line indices expected to be fold headers

  // clang-format off

  QTest::newRow("braces")
    << R"(
        module box() {
          cube(1);
        }
        x = 1;
      )"
    << QList{0, 1, 1, 0}
    << QList{0};

  QTest::newRow("brackets")
    << R"(
        x = [
          1,
          2
        ];
        y = 1;
      )"
    << QList{0, 1, 1, 1, 0}
    << QList{0};

  QTest::newRow("parens")
    << R"(
        translate(
          [1, 0, 0]
        ) cube(1);
        y = 1;
      )"
    << QList{0, 1, 1, 0}
    << QList{0};

  QTest::newRow("function_with_let")
    << R"(
        function foo(x) =
        let(
                a = 1
            )
            x + a;
        y = 1;
      )"
    << QList{0, 1, 2, 2, 1, 0}
    << QList{0, 1};

  QTest::newRow("function_single_line")
    << R"(
        function inc(x) = x + 1;
        y = 1;
      )"
    << QList{0, 0}
    << QList<int>{};

  QTest::newRow("function_with_object")
    << R"(
        function foo(x) =
          let(
              a = 1
          )
          object(
              key = a
          );
        y = 1;
      )"
    << QList{0, 1, 2, 2, 1, 2, 2, 0}
    << QList{0, 1, 4};

  // A more complex nesting case: brackets and parens that open *and* close
  // again within a single line should net to zero and not produce a spurious
  // fold header there, even though the surrounding module still folds on its
  // braces as usual.
  QTest::newRow("nested_module_call")
    << R"(
        module box(size=[1,1,1]) {
          translate([0,0,0])
            cube(size);
        }
        x = 1;
      )"
    << QList{0, 1, 1, 1, 0}
    << QList{0};

  // Regression guard: A semicolon inside a quoted string must not be
  // mistaken for the terminator of the enclosing 'function' definition.
  QTest::newRow("semicolon_in_string")
    << R"(
        function vector_font() = [
          [32, " ", /*...*/],
          [59, ";", /*...*/],
          [65, "A", /*...*/],
        ];
        x = 1;
      )"
    << QList{0, 2, 2, 2, 2, 0}
    << QList{0};

  // Regression guard: A semicolon inside a quoted string must not be
  // mistaken for the terminator of the enclosing 'function' definition.
  QTest::newRow("semicolon_in_comment")
    << R"(
        function semicolon_in_comment(a, b)
          = a // line comment; don't stop at the semicolon
          + b /* block comment; don't stop at the semicolon */
          ;
        x = 1;
      )"
    << QList{0, 1, 1, 1, 0}
    << QList{0};

  QTest::newRow("multiline_comment")
    << R"(
        x = 1; /*
        this is a
        multi-line comment
        */
        y = 2;
      )"
    << QList{0, 1, 1, 1, 0}
    << QList{0};

  // Combines the above with the function-closing logic: a multi-line comment
  // nested inside a function body, containing fake ');', ']', '}', and even
  // the literal word "function" -- none of which may be mistaken for the real
  // tokens they resemble. (Directly motivated by the sed false-positive we
  // found earlier, where the English word "function" inside a comment was
  // enough to fool a naive line-based approach.)
  QTest::newRow("multiline_comment_with_fake_tokens_in_function")
    << R"(
        function foo(x) =
          /*
          fake tokens: ); ] } and even the word function itself
          */
          x + 1;
        y = 2;
      )"
    << QList{0, 1, 2, 2, 1, 0}
    << QList{0, 1};

  // Negative case: a different Keyword-styled token ("echo") must NOT trigger
  // the function-virtual-level path. Folding here should come entirely from
  // ordinary paren-counting (patch 1), with zero contribution from the
  // function-specific branch.
  QTest::newRow("non_function_keyword_multiline")
    << R"(
        echo(
          "hello"
        );
        x = 1;
      )"
    << QList{0, 1, 1, 0}
    << QList{0};

  // Minimal sanity check: an empty comment opens and closes on the same
  // character transition and must net to zero, same as any other
  // same-line-balanced construct.
  QTest::newRow("empty_comment_single_line")
    << R"(
        x = 1; /**/ y = 2;
      )"
    << QList{0}
    << QList<int>{};

  // Two independent (non-nested) function definitions in sequence: confirms
  // startFunctionDef/currKeyword state is fully reset after the first
  // definition's terminating ';', so the second gets its own correct fold
  // with no leftover contamination from the first.
  QTest::newRow("sequential_functions")
    << R"(
        function first(x) =
          let(a = x + 1)
          a;
        function second(y) =
          let(b = y + 2)
          b;
        z = 1;
      )"
    << QList{0, 1, 1, 0, 1, 1, 0}
    << QList{0, 3};

  QTest::newRow("unfinished_before_function")
    << R"(
        function x(phi, r=1) =
          [for (phi = [0:15:360]) r * cos(phi)]

        function y(phi, r) =
          [for (phi = [0:15:360]) r * sin(phi)];

        x = 1;
      )"
    << QList{0, 1, 1, 1, 1, 0, 0}
    << QList{0};

  // clang-format on
}

void TestScadLexerFolding::testFolding()
{
  restoreWindowInitialState();
  window->designActionAutoReload->setChecked(false);  // only folding matters here

  QFETCH(QString, source);
  QFETCH(const QList<int>, expectedDepths);
  QFETCH(const QList<int>, expectedHeaders);

  auto *const editor = dynamic_cast<ScintillaEditor *>(window->activeEditor);
  QVERIFY(editor != nullptr);

  editor->setPlainText(source.trimmed());
  editor->resetHighlighting();

  QCOMPARE(lineCount(editor->qsci), expectedDepths.count());

  qInfo() << currentDepths(editor->qsci) << expectedDepths;
  qInfo() << currentHeaders(editor->qsci) << expectedHeaders;

  QCOMPARE(currentDepths(editor->qsci), expectedDepths);
  QCOMPARE(currentHeaders(editor->qsci), expectedHeaders);
}
