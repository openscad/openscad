#include "gui/parameter/GroupWidget.h"

#include <QLineEdit>
#include <QObject>
#include <QSizePolicy>
#include <QString>
#include <QStyleOptionToolButton>
#include <QStylePainter>
#include <QWidget>

#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
#include <QAccessibilityHints>
#include <QGuiApplication>
#include <QStyleHints>
#endif

GroupDisclosureButton::GroupDisclosureButton(QWidget *parent) : QToolButton(parent)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
  connect(QGuiApplication::styleHints()->accessibility(),
          &QAccessibilityHints::contrastPreferenceChanged, this, [this] { update(); });
#endif
}

void GroupDisclosureButton::paintEvent(QPaintEvent *event)
{
  // Preserve native disabled and high-contrast surfaces.
  bool useNativeSurface = !isEnabled();
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
  useNativeSurface |= QGuiApplication::styleHints()->accessibility()->contrastPreference() ==
                      Qt::ContrastPreference::HighContrast;
#endif
  if (useNativeSurface) {
    QToolButton::paintEvent(event);
    return;
  }

  QStyleOptionToolButton option;
  initStyleOption(&option);
  // Expansion remains the button's checked state, including for accessibility.
  // Paint it as a section header rather than a selected button.
  option.state &= ~QStyle::State_On;

  // Derive a subdued section surface from the current palette on every paint.
  // Qt permits deriving brushes from palette roles:
  // https://doc.qt.io/qt-6/style-drawing.html#the-palette
  const auto window = option.palette.color(QPalette::Window);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
  const auto accent = option.palette.color(QPalette::Accent);
#else
  const auto accent = option.palette.color(QPalette::Highlight);
#endif
  const qreal amount = option.state.testFlag(QStyle::State_Sunken) ? 0.08 :
                       option.state.testFlag(QStyle::State_MouseOver) ? 0.06 : 0.04;
  const auto surface = QColor::fromRgbF(
    window.redF() + amount * (accent.redF() - window.redF()),
    window.greenF() + amount * (accent.greenF() - window.greenF()),
    window.blueF() + amount * (accent.blueF() - window.blueF()));
  // Change only this paint's palette, so theme changes and child controls are unaffected.
  // Windows 11 uses an explicit Button brush for all states, hence the blends above.
  option.palette.setColor(QPalette::Button, surface);

  // Keep native borders, text, disclosure arrow, focus and DPI metrics.
  QStylePainter painter(this);
  painter.drawComplexControl(QStyle::CC_ToolButton, option);
}

GroupWidget::GroupWidget(const QString& title, QWidget *parent) : QWidget(parent)
{
  this->toggleButton.setText(title);
  this->toggleButton.setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
  this->toggleButton.setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  this->toggleButton.setCheckable(true);
  setExpanded(false);

  // don't waste space
  this->mainLayout.setVerticalSpacing(0);
  this->mainLayout.setContentsMargins(0, 0, 0, 0);
  this->contentArea.setContentsMargins(0, 0, 0, 0);
  this->contentLayout.setSpacing(0);
  this->contentLayout.setContentsMargins(0, 0, 0, 0);

  this->contentArea.setLayout(&contentLayout);
  this->contentArea.hide();
  this->mainLayout.addWidget(&toggleButton, 0, 0);
  this->mainLayout.addWidget(&contentArea, 1, 0);
  setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
  setLayout(&mainLayout);

  QObject::connect(&toggleButton, &QToolButton::toggled, this, &GroupWidget::setExpanded);
}

void GroupWidget::addWidget(QWidget *widget)
{
  contentLayout.addWidget(widget);
}

void GroupWidget::setExpanded(bool expanded)
{
  toggleButton.setChecked(expanded);
  toggleButton.setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
  if (expanded) {
    contentArea.show();
  } else {
    contentArea.hide();
  }
}
