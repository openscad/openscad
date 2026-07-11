#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextBrowser>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <string>
#include <vector>
#include <algorithm>

class DiffDialog : public QDialog
{
  Q_OBJECT
public:
  DiffDialog(const std::string& originalCode, const std::string& proposedCode, bool dark,
             QWidget *parent = nullptr)
    : QDialog(parent)
  {
    setWindowTitle(tr("Review Code Changes"));
    resize(800, 600);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    QLabel *descLabel =
      new QLabel(tr("The AI assistant has proposed the following changes. Deletions are shown on the "
                    "left (red), and additions on the right (green)."),
                 this);
    descLabel->setWordWrap(true);
    layout->addWidget(descLabel);

    QTextBrowser *browser = new QTextBrowser(this);
    browser->setReadOnly(true);
    layout->addWidget(browser);

    // Compute diff and generate HTML
    auto diff = compute_diff(split_lines(originalCode), split_lines(proposedCode));
    auto aligned = align_diff(diff);
    std::string html = generate_diff_html(aligned, dark);
    browser->setHtml(QString::fromStdString(html));

    // Checkbox for preview
    previewCheckbox = new QCheckBox(tr("Trigger preview after applying"), this);
    previewCheckbox->setChecked(true);

    // Button box
    QDialogButtonBox *buttonBox = new QDialogButtonBox(this);

    QPushButton *applyBtn = new QPushButton(tr("Apply Changes"), buttonBox);
    applyBtn->setStyleSheet(
      "background-color: #2563eb; color: white; font-weight: bold; border-radius: 4px; padding: 6px "
      "12px;");
    buttonBox->addButton(applyBtn, QDialogButtonBox::AcceptRole);

    QPushButton *discardBtn = new QPushButton(tr("Discard Changes"), buttonBox);
    discardBtn->setStyleSheet(
      "background-color: #ef4444; color: white; font-weight: bold; border-radius: 4px; padding: 6px "
      "12px;");
    buttonBox->addButton(discardBtn, QDialogButtonBox::DestructiveRole);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);

    connect(discardBtn, &QPushButton::clicked, this, [this]() {
      done(2);  // Custom status code 2 for Discard Changes
    });

    // Layout the checkbox and buttons side-by-side
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->addWidget(previewCheckbox);
    bottomLayout->addStretch(1);
    bottomLayout->addWidget(buttonBox);
    layout->addLayout(bottomLayout);
  }

  bool shouldTriggerPreview() const { return previewCheckbox && previewCheckbox->isChecked(); }

private:
  struct DiffElement {
    enum Type { UNCHANGED, DELETED, ADDED } type;
    std::string text;
  };

  struct AlignRow {
    enum Type { UNCHANGED, DIFF } type;
    std::string left_text;
    std::string right_text;
    bool left_deleted;
    bool right_added;
  };

  static std::vector<std::string> split_lines(const std::string& text)
  {
    std::vector<std::string> lines;
    std::string line;
    for (char c : text) {
      if (c == '\n') {
        lines.push_back(line);
        line.clear();
      } else if (c != '\r') {
        line.push_back(c);
      }
    }
    if (!line.empty() || text.empty() || text.back() == '\n') {
      lines.push_back(line);
    }
    return lines;
  }

  static std::vector<DiffElement> compute_diff(const std::vector<std::string>& A,
                                               const std::vector<std::string>& B)
  {
    int m = A.size();
    int n = B.size();
    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1, 0));
    for (int i = 1; i <= m; ++i) {
      for (int j = 1; j <= n; ++j) {
        if (A[i - 1] == B[j - 1]) {
          dp[i][j] = dp[i - 1][j - 1] + 1;
        } else {
          dp[i][j] = std::max(dp[i - 1][j], dp[i][j - 1]);
        }
      }
    }

    std::vector<DiffElement> result;
    int i = m, j = n;
    while (i > 0 || j > 0) {
      if (i > 0 && j > 0 && A[i - 1] == B[j - 1]) {
        result.push_back({DiffElement::UNCHANGED, A[i - 1]});
        i--;
        j--;
      } else if (j > 0 && (i == 0 || dp[i][j - 1] >= dp[i - 1][j])) {
        result.push_back({DiffElement::ADDED, B[j - 1]});
        j--;
      } else {
        result.push_back({DiffElement::DELETED, A[i - 1]});
        i--;
      }
    }
    std::reverse(result.begin(), result.end());
    return result;
  }

  static std::vector<AlignRow> align_diff(const std::vector<DiffElement>& diff)
  {
    std::vector<AlignRow> rows;
    size_t i = 0;
    while (i < diff.size()) {
      if (diff[i].type == DiffElement::UNCHANGED) {
        rows.push_back({AlignRow::UNCHANGED, diff[i].text, diff[i].text, false, false});
        i++;
      } else {
        std::vector<std::string> deleted;
        std::vector<std::string> added;
        while (i < diff.size() && diff[i].type != DiffElement::UNCHANGED) {
          if (diff[i].type == DiffElement::DELETED) {
            deleted.push_back(diff[i].text);
          } else if (diff[i].type == DiffElement::ADDED) {
            added.push_back(diff[i].text);
          }
          i++;
        }
        size_t max_lines = std::max(deleted.size(), added.size());
        for (size_t k = 0; k < max_lines; ++k) {
          std::string left = (k < deleted.size()) ? deleted[k] : "";
          std::string right = (k < added.size()) ? added[k] : "";
          bool left_del = (k < deleted.size());
          bool right_add = (k < added.size());
          rows.push_back({AlignRow::DIFF, left, right, left_del, right_add});
        }
      }
    }
    return rows;
  }

  static std::string escape_html(const std::string& src)
  {
    std::string dst;
    for (char c : src) {
      switch (c) {
      case '&':  dst += "&amp;"; break;
      case '<':  dst += "&lt;"; break;
      case '>':  dst += "&gt;"; break;
      case '"':  dst += "&quot;"; break;
      case '\'': dst += "&apos;"; break;
      default:   dst += c; break;
      }
    }
    return dst;
  }

  static std::string generate_diff_html(const std::vector<AlignRow>& rows, bool dark)
  {
    std::string html = "<html><body style='margin: 0; padding: 0;'>";
    html +=
      "<table style='width: 100%; border-collapse: collapse; font-family: monospace; font-size: 9pt; "
      "line-height: 1.3;'>";

    std::string bg_normal = dark ? "#1e293b" : "#ffffff";
    std::string fg_normal = dark ? "#f3f4f6" : "#1f2937";
    std::string border_color = dark ? "#334155" : "#e5e7eb";

    std::string bg_deleted = dark ? "#7f1d1d" : "#fee2e2";
    std::string fg_deleted = dark ? "#fca5a5" : "#991b1b";

    std::string bg_added = dark ? "#14532d" : "#dcfce7";
    std::string fg_added = dark ? "#86efac" : "#166534";

    std::string bg_empty = dark ? "#0f172a" : "#f9fafb";

    for (const auto& row : rows) {
      html += "<tr>";
      if (row.type == AlignRow::UNCHANGED) {
        std::string escaped = escape_html(row.left_text);
        html +=
          "<td colspan='2' style='width: 50%; white-space: pre-wrap; word-wrap: break-word; "
          "background-color: " +
          bg_normal + "; color: " + fg_normal + "; border-right: 1px solid " + border_color +
          "; padding: 2px 6px;'>" + (escaped.empty() ? " " : escaped) + "</td>";
        html +=
          "<td colspan='2' style='width: 50%; white-space: pre-wrap; word-wrap: break-word; "
          "background-color: " +
          bg_normal + "; color: " + fg_normal + "; padding: 2px 6px;'>" +
          (escaped.empty() ? " " : escaped) + "</td>";
      } else {
        // Left column
        if (row.left_deleted) {
          std::string escaped = escape_html(row.left_text);
          html += "<td style='width: 2%; text-align: center; font-weight: bold; background-color: " +
                  bg_deleted + "; color: " + fg_deleted + "; padding: 2px 2px;'>-</td>";
          html +=
            "<td style='width: 48%; white-space: pre-wrap; word-wrap: break-word; background-color: " +
            bg_deleted + "; color: " + fg_deleted + "; border-right: 1px solid " + border_color +
            "; padding: 2px 6px;'>" + (escaped.empty() ? " " : escaped) + "</td>";
        } else {
          html += "<td style='width: 2%; background-color: " + bg_empty + ";'></td>";
          html += "<td style='width: 48%; background-color: " + bg_empty + "; border-right: 1px solid " +
                  border_color + "; padding: 2px 6px;'></td>";
        }

        // Right column
        if (row.right_added) {
          std::string escaped = escape_html(row.right_text);
          html += "<td style='width: 2%; text-align: center; font-weight: bold; background-color: " +
                  bg_added + "; color: " + fg_added + "; padding: 2px 2px;'>+</td>";
          html +=
            "<td style='width: 48%; white-space: pre-wrap; word-wrap: break-word; background-color: " +
            bg_added + "; color: " + fg_added + "; padding: 2px 6px;'>" +
            (escaped.empty() ? " " : escaped) + "</td>";
        } else {
          html += "<td style='width: 2%; background-color: " + bg_empty + ";'></td>";
          html += "<td style='width: 48%; background-color: " + bg_empty + "; padding: 2px 6px;'></td>";
        }
      }
      html += "</tr>";
    }

    html += "</table></body></html>";
    return html;
  }

  QCheckBox *previewCheckbox = nullptr;
};
