#ifndef CLOADING_H
#define CLOADING_H


#include <QPushButton>
#include <QTimer>
#include <QLabel>
#include <QHBoxLayout>
class CLoading : public QWidget
{
	Q_OBJECT

public:
	CLoading(QWidget *parent = 0);
	~CLoading();
	void setText(QString strtxtshow);
	void loadingshow();
	void loadinghide();

public slots:
	void timefunc();
private:
	QTimer *timer;
	QPushButton* loadingbtn;
	QLabel* label;
	int count;
	QString txtshow;
};

#endif // CLOADING_H
