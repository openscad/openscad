
#pragma once

#include <QPoint>
#include <QString>
#include <optional>
#include "geometry/linalg.h"
#include "gui/QGLView.h"

namespace Measurement {

enum {
  MEASURE_IDLE,
  MEASURE_DIST1,
  MEASURE_DIST2,
  MEASURE_ANG1,
  MEASURE_ANG2,
  MEASURE_ANG3,
  MEASURE_HANDLE1,
  MEASURE_DIRTY
};

struct Result {
  struct Message {
    QString display_text;
    std::optional<QString> clipboard_text;
  };
  enum class Status { NoChange, Success, Error };

  Status status;

  /**
   * Reverse-ordered list of responses.
   */
  std::vector<Message> messages;

  void addText(QString str) { messages.push_back(Message{str}); }
  void addText(QString str, QString clipboard) { messages.push_back(Message{str, clipboard}); }
};

template <typename TView>
class Template
{
public:
  Template(void);
  void setView(TView *qglview);

  struct Distance {
    double distance;
    int line_count;
    std::optional<Eigen::Vector3d> ptDiff, toInfiniteLine, toEndpoint1, toEndpoint2;
  };

  /**
   * Advance the Measurement state machine.
   * @return When success or error, has reverse-ordered list of responses in `messages`.
   */
  Result statemachine(QPoint mouse);

  void startMeasureDistance(void);
  void startMeasureAngle(void);
  void startFindHandle(void);

  /**
   * Stops and cleans up measurement, if any in progress.
   * Safe to call multiple times.
   * @return True if measurement was in progress.
   */
  bool stopMeasure();

private:
  TView *qglview;
  Distance distMeasurement(SelectedObject& obj1, SelectedObject& obj2);
  // Should break out angle measurement for testing too!
  // Then un-reverse the messages because it's entirely unnecessary once all values are accessible.
};

using Measurement = Template<QGLView>;

struct FakeGLView {
  int measure_state = 0;
  std::vector<SelectedObject> selected_obj;
  std::vector<SelectedObject> shown_obj;

  std::vector<SelectedObject> selection_queue;

  void update() {}
  void selectPoint(int, int)
  {
    if (!selection_queue.empty()) {
      selected_obj.push_back(selection_queue.front());
      selection_queue.erase(selection_queue.begin());
    }
  }
};

};  // namespace Measurement
