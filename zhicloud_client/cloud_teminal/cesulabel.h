#ifndef CESULABEL_H
#define CESULABEL_H

#include <QWidget>
#include <QLabel>
class cesulabel : public QWidget
{
	Q_OBJECT

public:
	void setKBM(QString kb,QString m);
	cesulabel(QWidget *parent=NULL);
	~cesulabel();

private:
	QLabel* pjxz;
	QLabel* sudu;
	QLabel* xdy;
	QLabel* xm;
	QLabel* dk;
};

#endif // CESULABEL_H
