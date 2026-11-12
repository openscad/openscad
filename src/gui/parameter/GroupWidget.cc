#include "gui/parameter/GroupWidget.h"

#include <QLineEdit>
#include <QObject>
#include <QPen>
#include <QRectF>
#include <QSizePolicy>
#include <QString>
#include <QStyleOptionFocusRect>
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
  // Request hover repaints even when the native style does not use them.
  setAttribute(Qt::WA_Hover);
  // The macOS style manages its native focus ring in a separate QFocusFrame.
  setAttribute(Qt::WA_MacShowFocusRect);
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
  // Keep expansion out of the section header's visual state.
  option.state &= ~(QStyle::State_On | QStyle::State_Raised);

  // Derive a subdued section surface from the current palette on every paint.
  // Qt permits deriving brushes from palette roles:
  // https://doc.qt.io/qt-6/style-drawing.html#the-palette
  const auto window = option.palette.color(QPalette::Window);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
  const auto accent = option.palette.color(QPalette::Accent);
#else
  const auto accent = option.palette.color(QPalette::Highlight);
#endif
  // Use the Qt background palette, independent of the platform's theme setting.
  // Light palettes use 4/6/8% tint; dark palettes use 10/15/20%.
  const qreal baseAmount = window.lightnessF() < 0.5 ? 0.10 : 0.04;
  const qreal amount = baseAmount * (option.state.testFlag(QStyle::State_Sunken)      ? 2.0
                                     : option.state.testFlag(QStyle::State_MouseOver) ? 1.5
                                                                                      : 1.0);
  const auto surface = QColor::fromRgbF(window.redF() + amount * (accent.redF() - window.redF()),
                                        window.greenF() + amount * (accent.greenF() - window.greenF()),
                                        window.blueF() + amount * (accent.blueF() - window.blueF()));
  const auto foreground = option.palette.color(QPalette::WindowText);
  auto outline = foreground;
  outline.setAlphaF(outline.alphaF() * 0.12);

  // Native button surfaces can ignore Button brushes or omit unchecked panels.
  // Paint the same surface and border for expanded and collapsed sections ourselves.
  QStylePainter painter(this);
  const QRect buttonRect =
    style()->subControlRect(QStyle::CC_ToolButton, &option, QStyle::SC_ToolButton, this);
  const QRectF surfaceRect = QRectF(buttonRect).adjusted(2, 2, -2, -2);
  painter.save();
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setPen(QPen(outline, 1));
  painter.setBrush(surface);
  painter.drawRoundedRect(surfaceRect, 4, 4);
  painter.restore();

  // Other styles draw focus here; macOS uses WA_MacShowFocusRect instead.
  if (option.state.testFlag(QStyle::State_HasFocus)) {
    QStyleOptionFocusRect focus;
    focus.QStyleOption::operator=(option);
    focus.rect = buttonRect.adjusted(3, 3, -3, -3);
    focus.backgroundColor = surface;
    painter.drawPrimitive(QStyle::PE_FrameFocusRect, focus);
  }

  // Let the active Qt style draw the label and arrow using its font and metrics.
  // Use WindowText on the background derived from QPalette::Window (the Qt palette role).
  // These palette changes apply only to this paint operation.
  option.palette.setBrush(QPalette::ButtonText, option.palette.brush(QPalette::WindowText));
  option.state &= ~QStyle::State_Sunken;
  const int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, &option, this);
  option.rect = buttonRect.adjusted(frameWidth, frameWidth, -frameWidth, -frameWidth);
  painter.drawControl(QStyle::CE_ToolButtonLabel, option);
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
