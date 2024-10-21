#pragma once

#include <QWidget>
#include <boost/optional.hpp>
#include <vector>

#include "gui/qtgettext.h"
#include "ui_ParameterDescriptionWidget.h"
#include "core/customizer/ParameterObject.h"

enum class DescriptionStyle { ShowDetails, Inline, HideDetails, DescriptionOnly };

class ParameterDescriptionWidget : public QWidget, public Ui::ParameterDescriptionWidget
{
  Q_OBJECT

public:

  ParameterDescriptionWidget(QWidget *parent);
  void setDescription(ParameterObject *parameter, DescriptionStyle descriptionStyle);
};

class ParameterVirtualWidget : public QWidget
{
  Q_OBJECT

public:
  ParameterVirtualWidget(QWidget *parent, ParameterObject *parameter);
  ParameterObject *getParameter() const { return parameter; }
  virtual void setValue() = 0;
  // Parent container (ParameterWidget) notifies when preview is updated,
  // so that widgets with immediate AND delayed changes can keep track
  // and avoid emitting excess changed() signals.
  virtual void valueApplied() { }
  // Widgets which are immediate only (combobox and checkbox) don't need to keep track.

signals:
  // immediate tells customizer auto preview to skip timeout
  void changed(bool immediate);

private:
  ParameterObject *parameter;

protected:
  static int decimalsRequired(double value);
  static int decimalsRequired(const std::vector<double>& values);
};
