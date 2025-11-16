#pragma once
#include "UXTest.h"

class TestTabManager : public UXTest
{
  Q_OBJECT;
private slots:
  void initTestCase();
  void checkOpenClose();
  void checkReOpen();
};
