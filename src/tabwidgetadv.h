#ifndef FILE__TABWIDGETADV_H
#define FILE__TABWIDGETADV_H

#include <QPoint>
#include <QTabWidget>

class TabWidgetAdv : public QTabWidget
{
  Q_OBJECT
public:
  TabWidgetAdv(QWidget* parent = 0);
  ~TabWidgetAdv();

  int tabAt(QPoint pos);
};

#endif // FILE__TABWIDGETADV_H
