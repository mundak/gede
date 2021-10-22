#ifndef FILE__VARIABLEINFOWINDOW_H
#define FILE__VARIABLEINFOWINDOW_H

#include <QFont>
#include <QPainter>
#include <QString>
#include <QWidget>

class VariableInfoWindow : public QWidget
{
public:
  VariableInfoWindow(QFont* font);
  virtual ~VariableInfoWindow();

  void show(QString expr);
  void hide();

protected:
  void paintEvent(QPaintEvent* pe);
  void resizeEvent(QResizeEvent* re);

private:
  QString m_expr;
  QString m_text;
  QFont* m_font;
};

#endif // FILE__VARIABLEINFOWINDOW_H
