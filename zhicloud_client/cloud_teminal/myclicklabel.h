#ifndef MYCLICKLABEL_H
#define MYCLICKLABEL_H

#include <QLabel>
#include <QtGui>
#include <QtGlobal>
#include <QString>

class QLabel;
class QWidget;
class QEvent;


/*************************************************
Class Name： MyClickLabel
Description: 具有单击相应的标签类
*************************************************/
class MyClickLabel : public QLabel
{
	Q_OBJECT
public:
	explicit MyClickLabel(QWidget *parent = 0);
	MyClickLabel(const QString &text, QWidget *parent=0);
signals:
	// 鼠标单击信号
	void clicked();//IMClickLabel* label);

protected:
	// 鼠标单击事件
	void mouseReleaseEvent(QMouseEvent *);
	void enterEvent(QEvent *);
	void leaveEvent(QEvent *);
	public slots:

};

#endif // IMCLICKLABEL_H
